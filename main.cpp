/**
 * Copyright (c) 2017, Autonomous Networks Research Group. All rights reserved.
 * Developed by:
 * Autonomous Networks Research Group (ANRG)
 * University of Southern California
 * http://anrg.usc.edu/
 *
 * Contributors:
 * Jason A. Tran <jasontra@usc.edu>
 * Bhaskar Krishnamachari <bkrishna@usc.edu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to deal
 * with the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * - Redistributions of source code must retain the above copyright notice, this
 *     list of conditions and the following disclaimers.
 * - Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimers in the 
 *     documentation and/or other materials provided with the distribution.
 * - Neither the names of Autonomous Networks Research Group, nor University of 
 *     Southern California, nor the names of its contributors may be used to 
 *     endorse or promote products derived from this Software without specific 
 *     prior written permission.
 * - A citation to the Autonomous Networks Research Group must be included in 
 *     any publications benefiting from the use of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH 
 * THE SOFTWARE.
 */

/**
 * @file       main.cpp
 * @brief      Example using an ESP8266 to pub/sub using MQTT on MBED OS.
 * 
 *             This example is specifically built for the FIE workshop.
 *
 * @author     Jason Tran <jasontra@usc.edu>
 * @author     Bhaskar Krishnachari <bkrishna@usc.edu>
 * 
 */

#include "mbed.h"
#include "easy-connect.h"
#include "MQTTmbed.h"
#include "MQTTNetwork.h"
#include "MQTTClient.h"
#include "MailMsg.h"
#include "LEDThread.h"
#include "PrintThread.h"

extern "C" void mbed_reset();

/* connect this pin to both the CH_PD (aka EN) & RST pins on the ESP8266 just in case */
#define WIFI_HW_RESET_PIN       p26

/* Using a hostname instead of IP address has been unverified by us */
#define MQTT_BROKER_IPADDR      "192.168.29.91"
#define MQTT_BROKER_PORT        1883

/* turn on easy-connect debug prints */
#define EASY_CONNECT_LOGGING    true

DigitalOut wifiHwResetPin(WIFI_HW_RESET_PIN);
InterruptIn pushbutton(p8);

/* MQTTClient and TCPSocket (underneath MQTTNetwork) may not be thread safe. 
 * Lock this global mutex before any calls to publish(). 
 */
Mutex mqttMtx;

void pushbuttonCallback() {
    printf("button pushed\n");
    MailMsg *msg;

    /* send to LED thread which takes care of publishing blink fast commands */
    msg = getLEDThreadMailbox()->alloc();
    if (!msg) {
        printf("LEDThreadMailbox full!\n");
        return;
    }
    msg->content[0] = FWD_TO_LED_THR;
    msg->content[1] = LED_PUBLISH_BLINK_FAST;
    msg->length = 2;
    getLEDThreadMailbox()->put(msg);
}

/* Callback for any received MQTT messages */
void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    MailMsg *msg;

    /* our messaging standard says the first byte denotes which thread to fwd to */
    char fwdTarget = ((char *)message.payload)[0];

    /* Ship (or "dispatch") entire message via mail to threads since the 
       reference to messages will disappear soon after this callback returns */
    switch(fwdTarget)
    {
        case FWD_TO_PRINT_THR:
            printf("fwding to print thread\n");
            msg = getPrintThreadMailbox()->alloc();
            if (!msg) {
                printf("print thread mailbox full!\n");
                break;
            }
            memcpy(msg->content, message.payload, message.payloadlen);
            msg->length = message.payloadlen;
            getPrintThreadMailbox()->put(msg);
            break;
        case FWD_TO_LED_THR:
            printf("fwding to led thread\n");
            msg = getLEDThreadMailbox()->alloc();
            if (!msg) {
                printf("led thread mailbox full!\n");
                break;
            }
            memcpy(msg->content, message.payload, message.payloadlen);
            msg->length = message.payloadlen;
            getLEDThreadMailbox()->put(msg);
            break;
        default:
            /* do nothing */
            printf("Unknown MQTT message\n");
            break;
    }
}

int main()
{
    /* attach callback to pushbutton interrupt */
    pushbutton.mode(PullUp);
    pushbutton.rise(&pushbuttonCallback);

    wait(1); //delay startup 
    printf("Resetting ESP8266 Hardware...\n");
    wifiHwResetPin = 0;
    wait(1);
    wifiHwResetPin = 1;

    printf("Starting MQTT example with an ESP8266 wifi device using Mbed OS.\n");
    printf("Attempting to connect to access point...\n");

    /* wifi ssid/pw and wifi interface settings are set in mbed_app.json. 
     * NetworkInterface is mbed-OS's abstraction of any network interface */
    NetworkInterface* wifi = easy_connect(EASY_CONNECT_LOGGING);
    if (!wifi) {
        printf("Connection error! Your ESP8266 may not be responding.\n");
        printf("Try double checking your circuit or unplug/plug your LPC1768 board.\n");
        printf("Exiting.\n");
        return -1;
    }

    const char *ipAddr = wifi->get_ip_address();
    printf("Success! My IP addr: %s\n", ipAddr);
    char clientID[30];
    /* use ip addr as unique client ID */
    strcpy(clientID, ipAddr);

    /* wrapper for (NetworkInterface *wifi) to adapt to MQTTClient.h */
    MQTTNetwork mqttNetwork(wifi);
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    printf("Connecting to %s:%d\n", MQTT_BROKER_IPADDR, MQTT_BROKER_PORT);
    int retval = mqttNetwork.connect(MQTT_BROKER_IPADDR, MQTT_BROKER_PORT);
    if (retval != 0)
        printf("TCP connect returned %d\n", retval);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;   //support only available up to ver. 3
    // data.keepAliveInterval = 60; //MQTTPacket_connectData_initializer sets this to 60
    data.clientID.cstring = clientID; 
    if ((retval = client.connect(data)) != 0)
        printf("connect returned %d\n", retval);

#if MASTER_NODE
    char *topic = "mbed-wifi-master";
#else 
    char *topic = "mbed-wifi-example";
#endif

    /* define MQTTCLIENT_QOS2 as 1 to enable QOS2 (see MQTTClient.h) */
    /* Setup the callback to handle messages that arrive */
    if ((retval = client.subscribe(topic, MQTT::QOS0, messageArrived)) != 0)
        printf("MQTT subscribe returned %d\n", retval);

    /* launch threads */
    Thread ledThr;
    Thread printThr;

    /* pass in pointer to client so led thread can client.publish() messages */
    ledThr.start(callback(LEDThread, (void *)&client));
    printThr.start(printThread);


    /* The main thread will now run in the background to keep the MQTT/TCP 
     connection alive. MQTTClient is not an asynchronous library. Paho does
     have MQTTAsync, but some effort is needed to adapt mbed OS libraries to
     be used by the MQTTAsync library. */
    while(1) {
        Thread::wait(1000);
        printf("main: yielding...\n", client.isConnected());

        if(!client.isConnected())
            mbed_reset(); //connection lost! software reset

        /* yield() needs to be called at least once per keepAliveInterval */
        client.yield(1000);
    }

    return 0;
}