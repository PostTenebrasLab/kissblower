/*
 * =====================================================================================
 *
 *       Filename:  test_PTL_IR_remote_shield.h
 *
 *    Description:  kissblower (based on PTL_ir_remote)
 *
 *        Version:  1.0
 *        Created:  22/12/2018 19:25:31
 *       Revision:  none
 *       Compiler:  gcc-avr
 *
 *         Author:  Sebastien Chassot (sinux), seba.ptl@sinux.net
 *                  pliski, info@pli.ski
 *        Company:  Post Tenebras Lab (Geneva's Hackerspace)
 *
 * =====================================================================================
 */

#ifndef __TEST_PTL_IR_H__
#define __TEST_PTL_IR_H__

#define OTA
bool updateOTA();

#define ANALOG_PIN A0
#define LED_PIN 2

#define ESPID "XXXXXXXX"

#define FQDN(x) x ".local"
#define AP(x) x "_rescue"
#define PREFIX(x) "pub/" x

#define HOSTNAME FQDN(ESPID)
#define WIFI_AP_NAME AP(ESPID)
#define MQTTnamespace PREFIX(ESPID)
#define MQTT_RECONNECT_TIME 5000

#define MQTT_SERVER "iot.eclipse.org"
#define MQTT_PORT 1883
#define MQTT_USER "user"
#define MQTT_PASSWORD "1234"
#define MQTT_TOPIC "amazing_topic"

#define REFRESH_RATE 500

#endif //__TEST_PTL_IR_H__
