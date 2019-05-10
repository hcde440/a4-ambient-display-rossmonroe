/* Umbrella Holder
 *  
 * This program evaluates the distance of 2 ultrasonic sensors
 * then when within range, will grab date and time from an api
 * and package it into a message that is sent over mqtt.
 */

// ------------- LIBRARY INCLUDES -------------
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// ------------- WIFI SETTINGS -------------
#define ssid "WIFI"
#define pass "PASS"

// ------------- MQQT SETTINGS -------------
#define mqtt_server "broker.mqttdashboard.com"
#define mqtt_name "hcdeiot"
#define mqtt_pass ""
WiFiClient espClient;
PubSubClient mqtt(espClient);
unsigned long timer;
char espUUID[8] = "ESP8602";

// ------------- ULTRASONIC SENSORS -------------
#define ULTRASONIC_TRIG_1     12   // pin TRIG to D1
#define ULTRASONIC_ECHO_1     14   // pin ECHO to D2

#define ULTRASONIC_TRIG_2     13   // pin TRIG to D1
#define ULTRASONIC_ECHO_2     15   // pin ECHO to D2

// ------------- ULTRASONIC SENSORS DATA STRUCTURE -------------
typedef struct {
  int one; //Weather Conditions
  int two; //Current Time
  String tme;
} ultraSonic;

ultraSonic us; //allows the data to be passed to the struct through a call of traffic.

int triggerDistance = 80; //distance it takes to trigger the code to the mqtt server.

// The setup initializes the serial port, configures the wifi, starts the mqtt server, 
// a time, and finally sets the inputs and outputs for the ultrasonic sensors.
void setup() {
  Serial.begin(115200);
  configureWifi();
  
  mqtt.setServer(mqtt_server, 1883);

  timer = millis();
  
  // ultraonic setup 
  pinMode(ULTRASONIC_TRIG_1, OUTPUT);
  pinMode(ULTRASONIC_ECHO_1, INPUT);
  pinMode(ULTRASONIC_TRIG_2, OUTPUT);
  pinMode(ULTRASONIC_ECHO_2, INPUT);
  
}

//The lop actively evaluates both sensors. If they both are below a specified distance
//then it checks the mqtt connection, gets the current date and time, then publihs the
//date and time as well as an exit has been detected to the mqtt server.
void loop() {
  sensorOne();
  sensorTwo();
  if (us.one < triggerDistance && us.two < triggerDistance) {
     if (millis() - timer > 10000) { //a periodic report, every 10 seconds
      mqttConnectionCheck();
      getDate();
      publishSensors();
    }
  }
}

// ------------- WIFI SETUP -------------
void configureWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(); Serial.println("WiFi connected"); Serial.println();
  Serial.print("Your ESP has been assigned the internal IP address ");
  Serial.println(WiFi.localIP());
}

// ------------- MQTT CONNECTION CHECK -------------
void mqttConnectionCheck() {
  mqtt.loop();
  if (!mqtt.connected()) {
    reconnect();
  }
}


// ------------- RECONNECT TO MQTT -------------
void reconnect() {
  // Loop until we're reconnected
  while (!espClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    // Attempt to connect
    if (mqtt.connect(espUUID, mqtt_name, mqtt_pass)) { //the connction
      Serial.println("connected");
      // Once connected, publish an announcement...
      char announce[40];
      strcat(announce, espUUID);
      strcat(announce, "is connecting. <<<<<<<<<<<");
      mqtt.publish(espUUID, announce);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// ------------- DOWNLOAD DATE AND TIME -------------
//This API is a public one that requires no key so the url
//is very basic as well as the json parsing.
void getDate(){
  HTTPClient theClient;
  Serial.println();
  Serial.println("Making HTTP request to World Clock");
  theClient.begin("http://worldclockapi.com/api/json/pst/now");
  int httpCode = theClient.GET();
  if (httpCode > 0) {
    if (httpCode == 200) {
      Serial.println("Received HTTP payload.");
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      Serial.println("Parsing...");
      JsonObject& root = jsonBuffer.parse(payload);

      // Test if parsing succeeds.
      if (!root.success()) {
        Serial.println("parseObject() failed");
        Serial.println(payload);
        return;
      }
      us.tme = root["currentDateTime"].as<String>();
      Serial.println("==========");
      Serial.println(us.tme);
      Serial.println("==========");
    }
    else {
      Serial.println(httpCode);
      Serial.println("Something went wrong with connecting to the endpoint.");
    }
  }
}

// ------------- PUBLISH DATA TO MQTT -------------
//This function saves the exit detected boolean and date
// and time data to char. The date and time is limited
// to only include day and hours without the time zone
// designation to make parsing and formatting easier 
// on the other side.
void publishSensors() {
  char message[50];
  char dateTime[17];
  char exitDetected[7];
  String("true").toCharArray(exitDetected,7); //converts to char array
  String(us.tme).toCharArray(dateTime,17); //converts to char array
  
  sprintf(message, "{\"exit\": \"%s\", \"date_time\": \"%s\"}", exitDetected, dateTime);
  mqtt.publish("fromRoss/Sensors", message);
  Serial.println("publishing");
  Serial.println(message);
  timer = millis();
}


// ------------- ULTRASONIC SENSORS -------------
//The ultrasonic sensor functions are exactly the same
//but the serial print is commented out since it was
//only used for debugging. This triggers the sensors
//for 10 micro-seconds then turns it off and the length
//of the echo is then converted into centimeters.
void sensorOne(){
   long duration, distance;
   digitalWrite(ULTRASONIC_TRIG_1, LOW);  
   delayMicroseconds(2); 
   digitalWrite(ULTRASONIC_TRIG_1, HIGH);
   delayMicroseconds(10); 
   digitalWrite(ULTRASONIC_TRIG_1, LOW);
   duration = pulseIn(ULTRASONIC_ECHO_1, HIGH);
   distance = (duration/2) / 29.1;
   //Serial.print("1111111111 ------ Ultrasonic Distance: ");
   //Serial.print(distance);
   //Serial.println(" cm");
   us.one = distance;
}


void sensorTwo(){
   long duration, distance;
   digitalWrite(ULTRASONIC_TRIG_2, LOW);  
   delayMicroseconds(2);
   digitalWrite(ULTRASONIC_TRIG_2, HIGH);
   delayMicroseconds(10); 
   digitalWrite(ULTRASONIC_TRIG_2, LOW);
   duration = pulseIn(ULTRASONIC_ECHO_2, HIGH);
   distance = (duration/2) / 29.1;
   //Serial.print("2222222222 ------ Ultrasonic Distance: ");
   //Serial.print(distance);
   //Serial.println(" cm");
   us.two = distance;
}
