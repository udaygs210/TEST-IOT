#include <ESP8266WiFi.h>

#include <ESP8266mDNS.h>

#include <ESP8266WebServer.h>

#include <WiFiUdp.h> //Inbuilt ESP8266 Library

#include <ESP8266HTTPClient.h> //Inbuilt ESP8266 Library

#include <ArduinoJson.h> //From https://arduinojson.org/

#include <NTPClient.h> //From https://github.com/arduino-libraries/NTPClient

#include <PubSubClient.h>

ESP8266WebServer serverHTTP(80);
WiFiUDP udp;
NTPClient timeClient(udp);
WiFiClient espClient;
PubSubClient client(espClient);

String access_token = "";
String instance_url = "";
String deviceIp = "";

const char * sfLoginFingerprint = "BB:0C:5C:8A:78:79:D2:38:E6:F7:4C:13:FC:75:8D:93:87:67:18:75";
const char * sfInstanceFingerprint = "FF:BD:AD:C3:06:EA:EE:CC:72:D7:73:DB:C8:75:82:1C:45:37:D0:5A";
//eifi credentials
const char * ssid = "nacho-suite_2.5g";
const char * password = "333WafflePickleTomatoTomatoChickenPotPie";

// MQTT Broker
const char * mqtt_broker = "driver.cloudmqtt.com";
const char * topic = "light";
const char * topiclive = "livedata";
const char * topiclivestatus = "livestatus";
const char * topicslider = "slider";
const char * topiccasestaus = "casestatus";
const char * mqtt_username = "qvuhnrmt";
const char * mqtt_password = "4zQchS5ljNJU";
const int mqtt_port = 18923;

int button = D5;
int temp = 0;
int LED D0;
int analogInPin = A0;
int ledPin = D2;
int caseled = D3;
int sensorValue = 0;
String livestreamingstatus = "stop";
bool insertSuccess = false;
const char * deviceId = "02i4x000000l8D7";

bool connectToWifi() {
  byte timeout = 50;
  WiFi.begin(ssid, password);

  for (int i = 0; i < timeout; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      deviceIp = WiFi.localIP().toString();
      return true;
    }
    delay(5000);
  }
  return false;
}

void showWebpage() {

  String content = "<!DOCTYPE html><html>";
  content += "<head><title>ESP8266 Test Server</title>";
  content += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  content += "<style>body {font-family: sans-serif;} </style>";
  content += "</head><body>";
  content += "<p><a href=\"https://myiotpoc-developer-edition.na150.force.com/smartworld/services/oauth2/authorize?response_type=token&client_id=3MVG9kBt168mda_82eEUHTOtSaXXuZbRK2AuNv_hNT6uXgdgZUMxxbRYmlagqitmjmKsaknNSkNROY5JuNIHG&redirect_uri=https://gsdemochat-developer-edition.na150.force.com/liveAgentSetupFlow/apex/callbackpage&state=" + deviceIp + "\"><button class=\"button\">Connect with salesforce</button></a></p>";
  content += "<p>";
  time_t now = time(NULL);
  content += ctime( & now);
  content += "</p>";

  serverHTTP.send(200, "text/html", content);
}

void setup() {

  Serial.begin(115200);
  pinMode(button, INPUT_PULLUP); 
  pinMode(LED, OUTPUT); 
  pinMode(caseled, OUTPUT);
  if (!connectToWifi()) {
    delay(60000);
    ESP.restart();
  }

  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  serverHTTP.on("/", showWebpage);
  serverHTTP.on("/success", showsuccessWebpage);
  serverHTTP.begin();  
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Public emqx mqtt broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  // publish and subscribe
  client.subscribe(topic);
  client.subscribe(topiclivestatus);
  client.subscribe(topicslider);
  client.subscribe(topiccasestaus);

}
void showsuccessWebpage() {

  String message = "Number of args received:";
  message += serverHTTP.args(); //Get number of parameters
  message += "\n"; //Add a new line

  for (int i = 0; i < serverHTTP.args(); i++) {

    message += "Arg " + (String) i + " –> "; //Include the current iteration value
    message += serverHTTP.argName(i) + ": "; //Get the name of the parameter
    message += serverHTTP.arg(i) + "\n";
    if (serverHTTP.argName(i) == "access_token") {
      access_token = serverHTTP.arg(i);
    }
    if (serverHTTP.argName(i) == "instance_url") {
      instance_url = serverHTTP.arg(i);
    }
  }

  String content = "<!DOCTYPE html><html>";
  content += "<head><title>ESP8266 Test Server</title>";
  content += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  content += "<style>body {font-family: sans-serif;} </style>";
  content += "</head><body>";
  content += message;

  content += "<p>";
  time_t now = time(NULL);
  content += ctime( & now);
  content += "</p>";

  serverHTTP.send(200, "text/html", content);
  timeClient.begin();
  Serial.println("access_token>> " + access_token);

}

void callback(char * topic, byte * payload, unsigned int length) {

  String rt(topic);

  if (rt == "livestatus") {

    char message[length + 1];
    strncpy(message, (char * ) payload, length);
    message[length] = '\0';
    livestreamingstatus = message;

  }
  if (rt == "light") {

    if (!strncmp((char * ) payload, "1", length)) {
      digitalWrite(LED, HIGH);

    } else {
      digitalWrite(LED, LOW);

    }

  }

  if (rt == "casestatus") {
    if (!strncmp((char * ) payload, "0", length)) {
      digitalWrite(caseled, LOW);
      insertSuccess = false; //case closed 
    } else {
      digitalWrite(caseled, HIGH);
      insertSuccess = true;
    }
  }
  if (rt == "slider") {
    char message1[length + 1];
    strncpy(message1, (char * ) payload, length);
    message1[length] = '\0';
    Serial.println(message1);
    Serial.println("INSIDE SLIDER");
    String stringOne = String(message1);

    analogWrite(ledPin, stringOne.toInt());

  }

}

void loop() {
  temp = digitalRead(button);
  if (insertSuccess == false) {
    if (temp == HIGH) {

      Serial.println("BUTTON RELEASED");
      delay(100);

    } else {

      Serial.println("BUTTON PUSHED");
      JsonObject & event = buildCase();
      insertCase(event);
      delay(100);
    }
  }
  serverHTTP.handleClient();
  delay(100);

  sensorValue = analogRead(analogInPin);

  if (livestreamingstatus == "start") {
    client.publish(topiclive, String(sensorValue).c_str(), true);
  }

  client.loop();
  delay(250);

}

JsonObject & buildCase() {

  StaticJsonBuffer < 1024 > jsonBuffer;

  JsonObject & root = jsonBuffer.createObject();

  root["Status"] = "New";
  root["Subject"] = "Something Went wrong";
  root["Origin"] = "Web";
  root["Priority"] = "Medium";
  root["AssetId"] = deviceId;

  return root;

}

//create case
bool insertCase(JsonObject & object) {

  String reqURL = (String) instance_url + "/services/data/v51.0/sobjects/case";

  HTTPClient http;
  http.begin(reqURL, sfInstanceFingerprint);
  // http.begin(client, reqURL.c_str());
  http.addHeader("Authorization", "Bearer " + (String) access_token);
  http.addHeader("Content-Type", "application/JSON");

  String objectToInsert;
  object.printTo(objectToInsert);
  int httpCode = http.POST(objectToInsert);
  String payload = http.getString();

  http.end();

  if (httpCode == 201) {
    insertSuccess = true;
    digitalWrite(caseled, HIGH);

  }
  return insertSuccess;
  /************/
}
