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
#include "ESP8266Interface.h"

/* UART TX/RX Pin Settings */ 
#define MBED_WIFI_TX_PIN        p28
#define MBED_WIFI_RX_PIN        p27

/* connect this pin to both the CH_PD & RST pins on the ESP8266 just in case */
#define WIFI_HW_RESET_PIN       p26

#define WIFI_SSID    "IOT-DEMO"
#define WIFI_PASSWD  "i0twithARMandUSC"

/* turn on ESP8266 debug prints */
#define ESP8266_DEBUG   true

Serial pc(USBTX, USBRX, 115200);
ESP8266Interface wifi(MBED_WIFI_TX_PIN, MBED_WIFI_RX_PIN, ESP8266_DEBUG);
DigitalOut wifi_hw_reset(WIFI_HW_RESET_PIN);

int main()
{
    /* Hardware reset the ESP8266 */
    pc.printf("Resetting ESP8266 Hardware...\n");
    wifi_hw_reset = 0;
    wait(1);
    wifi_hw_reset = 1;

    pc.printf("Starting MQTT example with an ESP8266 wifi device using Mbed OS.\n");

    pc.printf("Attempting to connect to demo router...\n");

    int ret = wifi.connect(WIFI_SSID, WIFI_PASSWD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        pc.printf("Connection error! Your ESP8266 may not be responding.\n");
        pc.printf("Try double checking your circuit or unplug/plug your LPC1768 board.\n");
        pc.printf("Exiting.\n");
        return -1;
    }

    pc.printf("Success!\n");
    pc.printf("IP addr: %s\n", wifi.get_ip_address());

    wifi.disconnect();
    pc.printf("\nDone!\n");

    return 0;
}