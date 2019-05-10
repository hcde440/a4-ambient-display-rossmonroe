/*Program Description
 * This program listens for data from an mqtt server then grabs the date and time
 * from the repsonse, connects to an SSL api, grabs weather data, then parses it
 * to get the current weather conditions that are then sent to led's on a clock.
*/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

//Some immutable definitions/constants
#define ssid "WIFI"
#define pass "PASS"
#define mqtt_server "broker.mqttdashboard.com"
#define mqtt_name "hcdeiot"
#define mqtt_pass ""
WiFiClient espClient;
PubSubClient mqtt(espClient);
char mac[18];
typedef struct {
  bool ext;
} didLeave;
didLeave dl;

#include <Adafruit_NeoPixel.h>

#define ledPin 0
int ledTime = 5000;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(6, ledPin, NEO_GRB + NEO_KHZ800);

const char* host = "api.darksky.net";
const int httpsPort = 443;
String darkSkyKey = "API KEY";

// SHA1 fingerprint for dark sky SSL certificate
const char* fingerprint = "EA C3 0B 36 E8 23 4D 15 12 31 9B CE 08 51 27 AE C1 1D 67 2B";

String timeChange = "2019-05-08T17:00:00";
int timeCounter = 0;

int formattedTime;

unsigned long timer;

typedef struct {
  String con; //Weather Conditions
  String tme; //Current Time
} weatherData;

bool exitDetected = false;

weatherData weather; //allows the data to be passed to the struct through a call of traffic.


void wifiSetup(){
  Serial.print("Connecting to "); Serial.println(*ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(); Serial.println("WiFi connected"); Serial.println();
}


void setup() { //Initial setup to establish wifi connection, then calls two functions to retrieve API data from NASA and Map Quest.
  Serial.begin(115200);
  delay(10);

  wifiSetup();

  WiFi.macAddress().toCharArray(mac,18);
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback);
  timer = millis();
  strip.begin();
  strip.setBrightness(50);
  strip.show();
}


void loop() {
  mqttConnectionCheck();
  if (dl.ext == true){
      weatherCheck();
      weatherColor();
      dl.ext = false;
   }
}


// ------------- RECONNECT TO MQTT -------------
void mqttConnectionCheck() {
  mqtt.loop();
  if (!mqtt.connected()) {
    reconnect();
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_name, mqtt_pass)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("fromRoss/Sensors"); //we are subscribing to 'theTopic' and all subtopics below that topic
    } else {                        //please change 'theTopic' to reflect your topic you are subscribing to
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


// ------------- SERIAL PRINT MQTT SUBSCRIPTION -------------
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  DynamicJsonBuffer  jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  dl.ext = root["exit"];
  weather.tme = root["date_time"].as<String>();
  Serial.println(dl.ext);
  Serial.println(weather.tme);
}

// ------------- SERIAL PRINT MQTT SUBSCRIPTION -------------
//checks which is the current weather condition then sends color to the clock.
void weatherColor(){
  if (weather.con == "clear-day"){
    strip.fill((0,255,255), 0, 6);
    delay(ledTime);
    strip.show();
  }
  if (weather.con == "clear-night"){
      strip.fill((255,0,255), 0, 6);
      strip.show();
      delay(ledTime);
      strip.clear();
      Serial.println("Clear Night");
  }
  if (weather.con == "rain"){
    strip.Color(0, 0, 255);
    delay(ledTime);
    strip.show();
  }
  if (weather.con == "snow"){
    strip.Color(100, 100, 255);
    delay(ledTime);
    strip.show();
  }
}


//verifys SSL certificate, sends api call with current date and time, then recieves json weather report, parses out icon
//information for consistent repsonses and saves it to a weather condition variable from a data structure.
void weatherCheck() {
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }

  String url = "/forecast/6efc7d210a5d1a4262dec4d42ecd2cad/47.6062,-122.3321," + weather.tme + ":00?exclude=minutely,hourly,daily,alerts,flags";
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
//  Serial.println("reply was:");
//  Serial.println("==========");
//  Serial.println(line);
//  Serial.println("Received HTTP payload.");
      DynamicJsonBuffer jsonBuffer;
      String payload = line;
      Serial.println("Parsing...");
      JsonObject& root = jsonBuffer.parse(payload);
      if (!root.success()) {
        Serial.println("parseObject() failed");
        Serial.println(payload);
        return;
      }
      weather.con = root["currently"]["icon"].as<String>();
  Serial.println("==========");
  Serial.println(weather.con);
  Serial.println("==========");
  Serial.println("closing connection");
}
