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
#include <WS2812FX.h>

#include "credentials.h"
#include "config.h"
#include "ledstate.h"

char *user[] = {"mj", "rudi", "alissa", "sasa", "loic", "laurent", "greg", "cop", "pliski"};

#define DEBUG
#define LED_COUNT 1

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
        led(LED_STATE_WIFI_ERROR);
        // Reboot. A.k.a. "Have you tried turning it Off and On again?"
        ESP.reset();
        delay(1000);
    }
    if (!MDNS.begin(HOSTNAME))
    {
        led(LED_STATE_MQTT_ERROR);
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
long lastPotoTime = 0;
int lastPotoRead = 0;
char *currentUser = user[0];
long limitPotoTime = 500;
long potoDivider = 128;

void potoLoop()
{
    long now = millis();
    if (now - lastPotoTime > limitPotoTime)
    {
        lastPotoTime = now;
        int val = analogRead(Potentiometer) / potoDivider;

        String msg = "The resistance value is: ";
        msg = msg + val;
        msg = msg + ". lastPotoRead: " + lastPotoRead;
        char message[158];
        msg.toCharArray(message, 58);
        Serial.println(message);

        if (val != lastPotoRead)
        {
            Serial.println("val != lastPotoRead");
            lastPotoRead = val;
            currentUser = user[val];

            led(LED_STATE_USER_CHANGED);

            Serial.println(currentUser);
        }
    }
}
/* ---------------------------- Potentiometer End ----------------------*/

/**********************  LED AND BUTTON  ************************************/

// Variables will change:
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// int ledState = LOW;        // the current state of the output pin
int buttonState;           // the current reading from the input pin
int lastButtonState = LOW; // the previous reading from the input pin
// int lastLedState = LOW;
unsigned long ledInterval = 0;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long debounceDelay = 50;   // the debounce time; increase if the output flickers
unsigned long decayTime = 0;
unsigned long decayLimit = 2000;

void setupLedAndButton()
{
    Serial.println("setupLedAndButton");
    ledInit();
    pinMode(BUTTON_PIN, INPUT);
    // pinMode(LED_PIN, OUTPUT);

    // set initial LED state
    // digitalWrite(LED_PIN, ledState);
    Serial.println("setupLedAndButton end");
}

void ledUnSet()
{
    Serial.println("ledUnSet");
    ws2812fx.stop();
}

void led(int state)
{
    switch (state)
    {
    case LED_STATE_INIT:
        ledSet(FX_MODE_BLINK, CYAN, 255, 100, 200);
        break;
    case LED_STATE_WIFI_ERROR:
        ledSet(FX_MODE_FADE, RED, 190, 500, 2000);
        break;
    case LED_STATE_MQTT_ERROR:
        ledSet(FX_MODE_FADE, RED, 190, 2500, 1000);
        break;
    case LED_STATE_USER_CHANGED:
        ledSet(FX_MODE_STATIC, PURPLE, 120, 500, 100);
        break;
    case LED_STATE_BUTTON_PRESSED:
        ledSet(FX_MODE_COLOR_WIPE, YELLOW, 255, 500, 100);
        break;
    case LED_STATE_KISS_RX:
        ledSet(FX_MODE_RAINBOW_CYCLE, WHITE, BRIGHTNESS_MAX, 1000, 10000);
        break;
    default:
        ledUnSet();
        break;
    }
}

void ledSet(int mode, uint32_t color, uint8_t brightness, uint16_t speed, int interval)
{
    Serial.print("ledSet: ");
    Serial.println(mode);
    // Select an animation effect/mode. 0=static color, 1=blink, etc. You
    // can specify a number here, or there some handy keywords defined in
    // the WS2812FX.h file that are more descriptive and easier to remember.
    // ws2812fx.setMode(FX_MODE_BLINK);
    ws2812fx.setMode(mode);
    ws2812fx.setColor(color);           // Set the color of the LEDs
    ws2812fx.setBrightness(brightness); // Set the LED’s overall brightness. 0=strip off, 255=strip at full intensity
    ws2812fx.setSpeed(speed);           // Set the animation speed. 10=very fast, 5000=very slow
    ws2812fx.start();
    decayTime = millis();
    decayLimit = decayTime + interval;
}

void ledInit()
{
    Serial.println("ledInit");
    ws2812fx.init(); // Initialize the strip

    // Set the LED’s overall brightness. 0=strip off, 255=strip at full intensity
    ws2812fx.setBrightness(120);

    // Set the animation speed. 10=very fast, 5000=very slow
    ws2812fx.setSpeed(2000);

    ws2812fx.setColor(RED); // Set the color of the LEDs
    led(LED_STATE_INIT);
    // ws2812fx.start(); // Start the animation
    Serial.println("ledInit end");
}

void loopLedAndButton()
{
    // read the state of the switch into a local variable:
    int reading = digitalRead(BUTTON_PIN);

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
                led(LED_STATE_BUTTON_PRESSED);
            }
        }

        // // if the led state has changed:
        // if (ledState != lastLedState)
        // {
        //     lastLedState = ledState;
        //     Serial.print("ledState changed: ");
        //     Serial.println(ledState);
        //     if (ledState == HIGH)
        //     {
        //         decayTime = millis();
        //     }
        // }
    }

    if (decayTime > 0 && millis() > decayLimit)
    {
        decayTime = 0;
        ledUnSet();
    }

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
            return;
        }
    }
    Serial.println();
    Serial.println("LED_STATE_KISS_RX");
    led(LED_STATE_KISS_RX);
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
    ws2812fx.service();
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
