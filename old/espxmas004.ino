/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

#define wifi_ssid "PTL"
#define wifi_password "P0stT3n3br4sL4b"

#define mqtt_server "m21.cloudmqtt.com"
#define mqtt_port 16629
#define mqtt_user "dezesfus"
#define mqtt_password "simrswyZgXjh"

#define mqtt_topic "kiss"
#define client_name "pliski"

char *user[] = { "mj", "rudi",  "alissa", "sasa", "loic", "laurent", "greg", "cop", "pliski"};

// POTO
#define Potentiometer A0
int threhold=50; // you might need to adjust this value to define light on/off status
long lastPotoRead = 0;
long limitPotoRead = 500;
long potoDivider = 128;

void potoLoop(){
  long now = millis();
    if (now - lastPotoRead > limitPotoRead ) {
     lastPotoRead = now;
    int val=analogRead(Potentiometer) / potoDivider;
     String msg="The resistance value is: ";
     msg= msg+ val;
   if (val>threhold)
      msg="0: "+msg;//when the resistance value>50
    else
      msg="1: "+msg;//when the resistance value<50
     char message[58];
     msg.toCharArray(message,58);
     Serial.println(message);

      Serial.println(user[val]);
      
  }
}
// POTO end


// LED AND BUTTON
const int ledPin = 5;
const int buttonPin = 16;

// Variables will change:
int ledState = LOW;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
int lastLedState = LOW;  

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
unsigned long decayTime = 0;  
unsigned long decayLimit = 2000;  

void setupLedAndButton() {
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  // set initial LED state
  digitalWrite(ledPin, ledState);
}

//
//// log
// Serial.println("");
//  Serial.print("reading: ");
//  Serial.println(reading);
//  Serial.print("lastButtonState: ");
//  Serial.println(lastButtonState);
//  Serial.print("lastDebounceTime: ");
//  Serial.println(lastDebounceTime);
//  Serial.print("ledState: ");
//  Serial.println(ledState);
//  Serial.print("lastLedState: ");
//  Serial.println(lastLedState);
//  Serial.print("decayTime: ");
//  Serial.println(decayTime);
//  
void loopLedAndButton() {
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

    // If the led changed:
//  if (ledState != lastLedState) {
//    // reset the debouncing timer
//    lastDebounceTime = millis();
//    
//  }

  if ((millis() - lastDebounceTime) > debounceDelay) {

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;
      Serial.println("buttonState changed");
      if (buttonState == HIGH) {
        mqttSend();
      }
    }

    // if the led state has changed:
    if (ledState != lastLedState) {
      lastLedState = ledState;      
      Serial.print("ledState changed: ");
      Serial.println(ledState);
      if (ledState == HIGH) {
        decayTime = millis();
      }
    }
  }

  if(decayTime > 0 && millis() - decayTime > decayLimit){
    decayTime = 0;
    ledState = LOW;
  }

  // set the LED:
  digitalWrite(ledPin, ledState);

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
}



// LED AND BUTTON END


// MQTT

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
//int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println();
    
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    if(client_name[i] != (char)payload[i]){    
      Serial.println("payload not equal client name");
//      digitalWrite(ledPin, LOW);
      ledState = LOW;
      return;
    }
  }
  Serial.println();
  Serial.println("callback led high");
  ledState = HIGH;

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
//    String clientId = "ESP8266Client-";
//    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(client_name, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_topic, "hello world");
      // ... and resubscribe
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void mqttSend(){
    long now = millis();
    if (now - lastMsg > 2000) {
    lastMsg = now;
//    snprintf (msg, 50, "hello world #%ld", value);
//    Serial.print("Publish message: ");
//    Serial.println(msg);
    client.publish(mqtt_topic, client_name);
  }
}

// MQTT END


// MAIN

//unsigned int client_name_lenght = 0;

void setup() {
  Serial.begin(115200);
  
//  client_name_lenght = sizeof(client_name);
  setupLedAndButton();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  loopLedAndButton();

  potoLoop();

//  long now = millis();
//  if (now - lastMsg > 2000) {
//    lastMsg = now;
//    ++value;
//    snprintf (msg, 50, "hello world #%ld", value);
//    Serial.print("Publish message: ");
//    Serial.println(msg);
//    client.publish("outTopic", msg);
//  }
}
