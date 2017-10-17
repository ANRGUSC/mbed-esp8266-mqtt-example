/**
 * Copyright (c) 2017, Autonomous Networks Research Group. All rights reserved.
 * Developed by:
 * Autonomous Networks Research Group (ANRG)
 * University of Southern California
 * http://anrg.usc.edu/
 *
 * Contributors:
 * Jason A. Tran
 * Bhaskar Krishnamachari
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
 *             This example is specifically built for
 *
 * @author     Jason Tran <jason.tran@usc.edu>
 * @author     Bhaskar Krishnachari <bkrishna@usc.edu>
 * 
 */

#include "mbed.h"
#include "easy-connect.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

/* UART TX/RX Pin Settings */ 
#define MBED_WIFI_TX_PIN        p28
#define MBED_WIFI_RX_PIN        p27

/* connect this pin to both the CH_PD & RST pins on the ESP8266 just in case */
#define WIFI_HW_RESET_PIN       p26

#define WIFI_SSID           "IOT-DEMO"
#define WIFI_PASSWD         "i0twithARMandUSC"
#define MQTT_BROKER_IPADDR  "raspberrypi"

/* turn on  debug prints */
#define ESP8266_DEBUG           true
#define EASY_CONNECT_LOGGING    true

// ESP8266Interface wifi(MBED_WIFI_TX_PIN, MBED_WIFI_RX_PIN, ESP8266_DEBUG);
DigitalOut wifi_hw_reset(WIFI_HW_RESET_PIN);

int arrivedcount = 0;

void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
}

int main()
{
    wait(1); //delay startup 

    /* Hardware reset the ESP8266 */
    printf("Resetting ESP8266 Hardware...\n");
    wifi_hw_reset = 0;
    wait(1);
    wifi_hw_reset = 1;

    printf("Starting MQTT example with an ESP8266 wifi device using Mbed OS.\n");

    printf("Attempting to connect to demo router...\n");

    /* wifi settings in mbed_app.json */
    NetworkInterface* wifi = easy_connect(EASY_CONNECT_LOGGING);
    if (!wifi) {
        printf("Connection error! Your ESP8266 may not be responding.\n");
        printf("Try double checking your circuit or unplug/plug your LPC1768 board.\n");
        printf("Exiting.\n");
        return -1;
    }

    printf("Success!\n");
    const char *ip_addr = wifi->get_ip_address();
    printf("IP addr: %s\n", ip_addr);

    MQTTNetwork mqttNetwork(wifi);

    char* topic = "mbed-wifi-example";

    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    const char* hostname = MQTT_BROKER_IPADDR; 
    int port = 1883;
    printf("Connecting to %s:%d\r\n", hostname, port);
    int retval = mqttNetwork.connect(hostname, port);
    if (retval != 0)
        printf("TCP connect returned %d\r\n", retval);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = ip_addr; //use ip_addr for unique client name
    if ((retval = client.connect(data)) != 0)
        printf("connect returned %d\r\n", retval);

    /* define MQTTCLIENT_QOS2 as 1 to enable QOS2 (see MQTTClient.h) */
    if ((retval = client.subscribe(topic, MQTT::QOS0, messageArrived)) != 0)
        printf("MQTT subscribe returned %d\r\n", retval);

    MQTT::Message message;

    char buf[100];
    sprintf(buf, "Hello FIE!\r\n");
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf) + 1;
    retval = client.publish(topic, message);


    sprintf(buf, "Hello FIE! msg 2\r\n");
    message.qos = MQTT::QOS0;
    message.payloadlen = strlen(buf) + 1;
    retval = client.publish(topic, message);


    // QoS 2
    sprintf(buf, "Hello FIE! msg 3\r\n");
    message.qos = MQTT::QOS0;
    message.payloadlen = strlen(buf) + 1;
    retval = client.publish(topic, message);

    if ((retval = client.unsubscribe(topic)) != 0)
        printf("unsubscribe returned %d\r\n", retval);

    if ((retval = client.disconnect()) != 0)
        printf("disconnect returned %d\r\n", retval);

    mqttNetwork.disconnect();

    printf("finished %d msgs\r\n", arrivedcount);

    wifi->disconnect();
    printf("\nDone!\n");

    return 0;
}