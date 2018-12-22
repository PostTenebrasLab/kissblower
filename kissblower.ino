/*
 * =====================================================================================
 *
 *       Filename:  kissbower.ino
 *
 *    Description:  kissblower (based on PTL_ir_remote).
 *                  blow kisses to your beloveds from your esp powered device through mqtt
 *
 *        Version:  1.0
 *        Created:  22/12/2018 19:25:31
 *       Revision:  none
 *       Compiler:  gcc-avr
 *
 *         Author:  pliski, info@pli.ski
 *        Company:  Post Tenebras Lab (Geneva's Hackerspace)
 *
 * =====================================================================================
 */

#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>

#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "config.h"

char *user[] = {"mj", "rudi", "alissa", "sasa", "loic", "laurent", "greg", "cop", "pliski"};

#define DEBUG

long timer_serial = 0;

/**********************  WIFI and Network ***********************/
extern const char index_html[];

extern const char main_js[];

#define WIFI_TIMEOUT 30000 // checks WiFi every ...ms. Reset after this time, if WiFi cannot reconnect.
#define HTTP_PORT 80

extern const char index_html[];
extern const char main_js[];

WiFiManager wifiManager;
ESP8266WebServer server(HTTP_PORT);

// QUICKFIX...See https://github.com/esp8266/Arduino/issues/263
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

/**
 * 
 */
void wifi_setup()
{
    delay(10);

    // We start by connecting to a WiFi network
    wifiManager.setTimeout(300); // Time out after 5 mins.

    Serial.print("WIFI_AP_NAME:\t");
    Serial.println(WIFI_AP_NAME);

    if (!wifiManager.autoConnect(WIFI_AP_NAME))
    {
        Serial.print("Wifi failed to connect and hit timeout.");
        // Reboot. A.k.a. "Have you tried turning it Off and On again?"
        ESP.reset();
        delay(1000);
    }
    if (!MDNS.begin(HOSTNAME))
    {
        Serial.println("Error setting up MDNS responder!");
    }
    Serial.print("WiFi connected. IP address:\t");
    Serial.println(WiFi.localIP().toString());
    Serial.print("WiFi connected. MAC address:\t");
    Serial.println(WiFi.macAddress());
}

/* ---------------------------- WIFI and Network End ----------------------*/

/**********************  Potentiometer  ************************************/

#define Potentiometer A0
int threhold = 50; // you might need to adjust this value to define light on/off status
long lastPotoRead = 0;
long limitPotoRead = 500;
long potoDivider = 128;

void potoLoop()
{
    long now = millis();
    if (now - lastPotoRead > limitPotoRead)
    {
        lastPotoRead = now;
        int val = analogRead(Potentiometer) / potoDivider;
        String msg = "The resistance value is: ";
        msg = msg + val;
        if (val > threhold)
            msg = "0: " + msg; //when the resistance value>50
        else
            msg = "1: " + msg; //when the resistance value<50
        char message[58];
        msg.toCharArray(message, 58);
        Serial.println(message);

        Serial.println(user[val]);
    }
}
/* ---------------------------- Potentiometer End ----------------------*/

/**********************  LED AND BUTTON  ************************************/

const int ledPin = 5;
const int buttonPin = 16;

// Variables will change:
int ledState = LOW;        // the current state of the output pin
int buttonState;           // the current reading from the input pin
int lastButtonState = LOW; // the previous reading from the input pin
int lastLedState = LOW;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long debounceDelay = 50;   // the debounce time; increase if the output flickers
unsigned long decayTime = 0;
unsigned long decayLimit = 2000;

void setupLedAndButton()
{
    pinMode(buttonPin, INPUT);
    pinMode(ledPin, OUTPUT);

    // set initial LED state
    digitalWrite(ledPin, ledState);
}

void loopLedAndButton()
{
    // read the state of the switch into a local variable:
    int reading = digitalRead(buttonPin);

    // check to see if you just pressed the button
    // (i.e. the input went from LOW to HIGH), and you've waited long enough
    // since the last press to ignore any noise:

    // If the switch changed, due to noise or pressing:
    if (reading != lastButtonState)
    {
        // reset the debouncing timer
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {

        // if the button state has changed:
        if (reading != buttonState)
        {
            buttonState = reading;
            Serial.println("buttonState changed");
            if (buttonState == HIGH)
            {
                mqttSend();
            }
        }

        // if the led state has changed:
        if (ledState != lastLedState)
        {
            lastLedState = ledState;
            Serial.print("ledState changed: ");
            Serial.println(ledState);
            if (ledState == HIGH)
            {
                decayTime = millis();
            }
        }
    }

    if (decayTime > 0 && millis() - decayTime > decayLimit)
    {
        decayTime = 0;
        ledState = LOW;
    }

    // set the LED:
    digitalWrite(ledPin, ledState);

    // save the reading. Next time through the loop, it'll be the lastButtonState:
    lastButtonState = reading;
}

/* ---------------------------- LED AND BUTTON End ----------------------*/

/*****************************  MQTT  ************************************/
// IPAddress mqttserver(34, 251, 42, 52);
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println();

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
        if (ESPID[i] != (char)payload[i])
        {
            Serial.println("payload not equal client name");
            ledState = LOW;
            return;
        }
    }
    Serial.println();
    Serial.println("callback led high");
    ledState = HIGH;
}

void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        //    String clientId = "ESP8266Client-";
        //    clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(ESPID, MQTT_USER, MQTT_PASSWORD))
        {
            Serial.println("connected");
            // Once connected, publish an announcement...
            client.publish(MQTT_TOPIC, "hello world");
            // ... and resubscribe
            client.subscribe(MQTT_TOPIC);
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void mqttSend()
{
    long now = millis();
    if (now - lastMsg > 2000)
    {
        lastMsg = now;
        //    snprintf (msg, 50, "hello world #%ld", value);
        //    Serial.print("Publish message: ");
        //    Serial.println(msg);
        client.publish(MQTT_TOPIC, ESPID);
    }
}

/* ---------------------------- MQTT END ----------------------*/

/*****************************  MAIN  ************************************/
void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    Serial.println("Starting...");

    setupLedAndButton();

    Serial.println("Wifi setup");
    wifi_setup();

    Serial.println("HTTP server setup");
    server.on("/", srv_handle_index_html);
    server.on("/main.js", srv_handle_main_js);
    server.on("/hello", srv_handle_hello);
    server.onNotFound(srv_handle_not_found);
    server.begin();
    Serial.println("HTTP server started.");

    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);

    Serial.println("ready!");
}

void loop()
{
    unsigned long now = millis();

    server.handleClient();

    client.loop();

    if (millis() > timer_serial)
    {

        if (!client.connected())
        {
            reconnect();
        }

#ifdef DEBUG
        Serial.println("tic");
#endif

        loopLedAndButton();
        potoLoop();

        timer_serial = millis() + REFRESH_RATE;
    }
}

/* #####################################################
#  Webserver Functions
##################################################### */

void srv_handle_not_found()
{
    server.send(404, "text/plain", "File Not Found");
}

void srv_handle_index_html()
{
    server.send_P(200, "text/html", index_html);
}

void srv_handle_main_js()
{
    server.send_P(200, "application/javascript", main_js);
}

void srv_handle_hello()
{
    server.send(200, "text/plain", "OK");
}
