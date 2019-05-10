#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>            

//Some immutable definitions/constants
#define ssid "Monroe"
#define pass "borstad1961"
#define mqtt_server "broker.mqttdashboard.com"
#define mqtt_name "hcdeiot"
#define mqtt_pass ""
WiFiClient espClient;
PubSubClient mqtt(espClient);
char mac[18];
typedef struct {
  bool ext; //Weather Conditions
} didLeave;
didLeave dl;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  DynamicJsonBuffer  jsonBuffer; //blah blah blah a DJB
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  dl.ext = root["exit"];
  Serial.println(dl.ext);
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


void setup()
{
  Serial.begin(115200);//for debugging code, comment out for production

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(); Serial.println("WiFi connected"); Serial.println();
  Serial.print("Your ESP has been assigned the internal IP address ");
  Serial.println(WiFi.localIP());

  WiFi.macAddress().toCharArray(mac,18);

    //connects to MQTT server
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback);
}

void loop() {
 if (dl.ext == true){
  Serial.println("FUCK YOU BITCH");
  dl.ext = false;
 }
 mqtt.loop();
  if (!mqtt.connected()) {
    reconnect();
  }
}
