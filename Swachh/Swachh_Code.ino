#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

#define S0 D0                             /* Assign Multiplexer pin S0 connect to pin D0 of NodeMCU */
#define S1 D1                             /* Assign Multiplexer pin S1 connect to pin D1 of NodeMCU */
#define S2 D2                             /* Assign Multiplexer pin S2 connect to pin D2 of NodeMCU */
#define S3 D3                             /* Assign Multiplexer pin S3 connect to pin D3 of NodeMCU */
#define SIG A0                            /* Assign SIG pin as Analog output for all 16 channels of Multiplexer to pin A0 of NodeMCU */
/* Ultrasonic*/
#define trig D5
#define echo D6

long tankDepth=19;



const char* ssid = "PRO";
const char* password = "123456789";
const char* mqtt_server = "192.168.16.159";


int decimal = 2;                          // Decimal places of the sensor value outputs 
int tds;                            /* Assign the name "sensor0" as analog output value from Channel C0 */
int tss;
int ph = 0;                            /* Assign the name "sensor1" as analog output value from Channel C1 */


float tdsVal = 0.0F;
float tdsValue = 0;
int tssVal = 0;
int resval = 0;
float volume, lastTime;
volatile long pulse;

float calibration_value = 21.34-0.5;
int phval = 0; 
unsigned long int avgval; 
int buffer_arr[10],temp;
 
float ph_act;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(100);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

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

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("/mh3/sensors", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void setup() {                                /* Put your codes here to run only once during micro controller startup */

  Wire.begin();  
  pinMode(S0,OUTPUT);                       /* Define digital signal pin as output to the Multiplexer pin SO */        
  pinMode(S1,OUTPUT);                       /* Define digital signal pin as output to the Multiplexer pin S1 */  
  pinMode(S2,OUTPUT);                       /* Define digital signal pin as output to the Multiplexer pin S2 */ 
  pinMode(S3,OUTPUT);                       /* Define digital signal pin as output to the Multiplexer pin S3 */  
  pinMode(SIG, INPUT);                      /* Define analog signal pin as input or receiver from the Multiplexer pin SIG */  
  
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);


  Serial.begin(115200);                       /* to display readings in Serial Monitor at 9600 baud rates */

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {                                
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  StaticJsonDocument<64> doc;
  char output[55];

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    // Channel 0 (C0 pin - binary output 0,0,0,0)
    digitalWrite(S0,LOW); digitalWrite(S1,LOW); digitalWrite(S2,LOW); digitalWrite(S3,LOW);
    tdsVal = analogRead(SIG);
  
    // // Channel 1 (C1 pin - binary output 1,0,0,0)
    digitalWrite(S0,HIGH); digitalWrite(S1,LOW); digitalWrite(S2,LOW); digitalWrite(S3,LOW);
    tss = analogRead(SIG);

    // digitalWrite(S0,HIGH); digitalWrite(S1,LOW); digitalWrite(S2,HIGH); digitalWrite(S3,LOW);
    // ph= analogRead(SIG);




for(int i=0;i<10;i++) { 
  digitalWrite(S0,HIGH); digitalWrite(S1,LOW); digitalWrite(S2,HIGH); digitalWrite(S3,LOW);
  buffer_arr[i]=analogRead(SIG);
  delay(30);
}
for(int i=0;i<9;i++){
  for(int j=i+1;j<10;j++){
    if(buffer_arr[i]>buffer_arr[j]){
      temp=buffer_arr[i];
      buffer_arr[i]=buffer_arr[j];
      buffer_arr[j]=temp;
    }
  }
}
avgval=0;
for(int i=2;i<8;i++)
  avgval+=buffer_arr[i];
float volt=(float)avgval*5.0/1024/6; 
ph_act = -5.70 * (volt-0.27) + calibration_value;


  digitalWrite(trig,LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long t = pulseIn(echo, HIGH);
  long cm = t / 29 / 2;
  // Serial.println(cm);
  double level= tankDepth-cm;
  // if(level>0){
  //   long percentage=((level/tankDepth))*100;
  //   Serial.print(percentage+" %");
  //   // Blynk.virtualWrite(V0,percentage);
  // }
  delay(100); 


  
   

    doc["ph"] = ph_act;
    doc["tds"] = tdsVal;
    doc["tss"] = tss;
    doc["lev"] = level;

    Serial.println("Read");
    serializeJson(doc, output);
    Serial.println(output);
    client.publish("/mh3/sensors",output);
    Serial.println("Sent");
  }
  


}