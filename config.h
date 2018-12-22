/*
 * =====================================================================================
 *
 *       Filename:  test_PTL_IR_remote_shield.h
 *
 *    Description:  PTL_ir_remote running on Wemos (esp8266 D1)
 *                  esp8266 IR remote control with web server and MQTT (reder/writer)
 *
 *        Version:  1.0
 *        Created:  04/07/2018 11:12:31
 *       Revision:  none
 *       Compiler:  gcc-avr
 *
 *         Author:  Sebastien Chassot (sinux), seba.ptl@sinux.net
 *        Company:  Post Tenebras Lab (Geneva's Hackerspace)
 *
 * =====================================================================================
 */

#ifndef __PTL_KISS_CONFIG__
#define __PTL_KISS_CONFIG__

#define OTA
bool updateOTA();

#define ANALOG_PIN A0
#define LED_PIN 5
#define BUTTON_PIN 16

#define FQDN(x) x ".local"
#define AP(x) x "_rescue"
#define PREFIX(x) "pub/" x

#define HOSTNAME FQDN(ESPID)
#define WIFI_AP_NAME AP(ESPID)
#define MQTTnamespace PREFIX(ESPID)
#define MQTT_RECONNECT_TIME 5000

#define FW_UPDATE_URL "BBBB" // ???

#define REFRESH_RATE 500

#endif //__PTL_KISS_CONFIG
