#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <ESP8266WiFi.h>


#include "FS.h"
//#include &lt;ESP8266WiFi.h&gt;
#include <PubSubClient.h> //https://www.arduinolibraries.info/libraries/pub-sub-client
#include <NTPClient.h> //https://www.arduinolibraries.info/libraries/ntp-client
#include <WiFiUdp.h>

char ssid[] = "MASS";                       // Your WiFi credentials.
char password[] = "9483527761";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
const char* AWS_endpoint = "akbxv3nuanuv3-ats.iot.ap-south-1.amazonaws.com";

const int MPU_addr=0x68;

int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
int minVal=265;
int maxVal=402;
double x;
double y;
double z;

Adafruit_MPU6050 mpu;

void callback(char* topic, byte* payload, unsigned int length) {
Serial.print("Message arrived [");
Serial.print(topic);
Serial.print("] ");

for (int i = 0; i < length; i++) {
Serial.print((char)payload[i]);
}

Serial.println();
}


WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient); //set MQTT port number to 8883 as per //standard

//============================================================================
#define BUFFER_LEN 256
long lastMsg = 0;
char msg[BUFFER_LEN];
int value = 0;
byte mac[6];
char mac_Id[18];

//============================================================================
void setup_wifi() {
delay(10);
// We start by connecting to a WiFi network
espClient.setBufferSizes(512, 512);
Serial.println();
Serial.print("Connecting to ");
Serial.println(ssid);
WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}

Serial.println("");
Serial.println("WiFi connected");
Serial.println("IP address: ");
Serial.println(WiFi.localIP());
timeClient.begin();

while(!timeClient.update()){
timeClient.forceUpdate();
}

espClient.setX509Time(timeClient.getEpochTime());
}

void reconnect() {
// Loop until we're reconnected
while (!client.connected()) {
Serial.print("Attempting MQTT connection...");
// Attempt to connect
if (client.connect("ESPthing")) {
Serial.println("connected");
// Once connected, publish an announcement...
client.publish("outTopic", "hello world");
// ... and resubscribe
client.subscribe("inTopic");
} else {
Serial.print("failed, rc=");
Serial.print(client.state());
Serial.println(" try again in 5 seconds");
char buf[256];
espClient.getLastSSLError(buf,256);
Serial.print("WiFiClientSecure SSL error: ");
Serial.println(buf);
// Wait 5 seconds before retrying
delay(5000);
}
}
}


void setup(){
Serial.begin(9600);
if (!mpu.begin()) {
  Serial.println("Sensor init failed");
  while (1)
    yield();
}
mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
mpu.setGyroRange(MPU6050_RANGE_500_DEG);
mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);


setup_wifi();
delay(1000);
if (!SPIFFS.begin()) {
Serial.println("Failed to mount file system");
return;
 }

Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
// Load certificate file
File cert = SPIFFS.open("/cert.der", "r"); //replace cert.crt eith your uploaded file name
if (!cert) {
Serial.println("Failed to open cert file");
}
else
Serial.println("Success to open cert file");
delay(1000);
if (espClient.loadCertificate(cert))   // downgrade nodemcu to 2.7.4 https://stackoverflow.com/questions/67962746/class-bearsslwificlientsecure-has-no-member-named-loadcertificate
Serial.println("cert loaded");
else
Serial.println("cert not loaded");
// Load private key file
File private_key = SPIFFS.open("/private.der", "r"); //replace private eith your uploaded file name
if (!private_key) {
Serial.println("Failed to open private cert file");
}
else
Serial.println("Success to open private cert file");
delay(1000);
if (espClient.loadPrivateKey(private_key))
Serial.println("private key loaded");
else
Serial.println("private key not loaded");
// Load CA file
File ca = SPIFFS.open("/ca.der", "r"); //replace ca eith your uploaded file name
if (!ca) {
Serial.println("Failed to open ca ");
}
else
Serial.println("Success to open ca");
delay(1000);
if(espClient.loadCACert(ca))
Serial.println("ca loaded");
else
Serial.println("ca failed");
Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
//===========================================================================
WiFi.macAddress(mac);
snprintf(mac_Id, sizeof(mac_Id), "%02x:%02x:%02x:%02x:%02x:%02x",
mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
Serial.print(mac_Id);
//============================================================================
}

 void loop(){

if (!client.connected()) {
reconnect();
}
client.loop();

sensors_event_t a, g, temp;
mpu.getEvent(&a, &g, &temp);

long now = millis();

if (now - lastMsg > 2000) {
lastMsg = now;

String macIdStr = mac_Id;
uint8_t angle = 30;  //TODO get a value from previous block
String randomString = String(random(0xffff), HEX);
snprintf (msg, BUFFER_LEN, "{\"mac_Id\" : \"%s\", \"angle\" : %d, \"random_string\" : \"%s\"}", macIdStr.c_str(), angle, randomString.c_str());

Serial.print("Publish message: ");
Serial.println(msg);
//mqttClient.publish("outTopic", msg);
client.publish("outTopic", msg);
//=============================================================================
Serial.print("Heap: "); Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
}
 
delay(1000);
}
