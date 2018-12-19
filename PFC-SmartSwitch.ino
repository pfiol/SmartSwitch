#include <ESP8266WiFi.h>
#include <MQTTClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

/* ---------- DO NOT EDIT ANYTHING ABOVE THIS LINE ---------- */

//Only edit the settings in this section

/* WIFI Settings */
// Name of wifi network
const char* ssid = "TP-Link_3700";

// Password to wifi network
const char* password = "18964196"; 

/* Web Updater Settings */
// Host Name of Device
const char* host = "SwitchSensor";

// Path to access firmware update page (Not Neccessary to change)
const char* update_path = "/firmware";

// Username to access the web update page
const char* update_username = "admin";

// Password to access the web update page
const char* update_password = "admin";

/* MQTT Settings */
// Topic which listens for commands
char* outTopic = "Home/electricity/SwitchSensor"; // MODIFY THIS TO YOUR CONVINIENCE 

//MQTT Server IP Address
const char* server = "192.168.0.20";  // YOUR MQTT BROKER IP ADDRESS GOES HERE

//Unique device ID 
const char* mqttDeviceID = "MySmartSwitch"; // YOUR UNIQUE MQTT DEVICE ID GOES HERE

char* currentState = "CLOSED";
/* ---------- DO NOT EDIT ANYTHING BELOW THIS LINE ---------- */

//the time when the sensor outputs a low impulse
long unsigned int lowIn;         

//the amount of milliseconds the sensor has to be low 
//before we assume all detection has stopped
long unsigned int pause = 100;  

//sensor variables
boolean lockLow = true;
boolean takeLowTime;  

//the digital pin connected to the door sensor's output
int sensorPin = 2;  

//webserver 
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

//MQTT 
WiFiClient net;
MQTTClient client;

//Time Variable
unsigned long lastMillis = 0;

//Connect to WiFI and MQTT
void connect();

//Setup pins, wifi, webserver and MQTT
void setup() 
{
  Serial.begin(115200);
  
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, LOW);
  Serial.print("\n\r \n\rPins SET!");
  Serial.print("\n\r \n\rWIFI work");
  WiFi.mode(WIFI_STA);
  Serial.print("\n\r \n\rWorking to connect");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  client.begin(server, net);
  client.onMessage(messageReceived);

  connect();
  Serial.print("\n\r \n\rConnected!");
  MDNS.begin(host);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.on("/toggleLight", []() {
    if(!client.connected()) {
        connect();
    } 

    if(currentState == "CLOSED"){
      client.publish(outTopic, "OPEN");
      currentState = "OPEN";
    }else{
      client.publish(outTopic, "CLOSED");
      currentState = "CLOSED";
    }
    
    httpServer.send(200, "text/plain", "Done!");

  });

  httpServer.on("/reset", []() {
    ESP.reset();
  });
  
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
}

//Connect to wifi and MQTT
void connect() 
{
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    WiFi.begin(ssid, password);
  }

  
  while (!client.connect(mqttDeviceID)) 
  {
    delay(1000);
  }
  Serial.println("is connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() 
{
  // MQTT Loop
  client.loop();
  delay(10);

  httpServer.handleClient();

  //Sensor Detection
  
  if(digitalRead(sensorPin) == HIGH)
  {
    //Serial.println("is high");
    if(lockLow)
    {  
      //makes sure we wait for a transition to LOW before any further output is made:
      lockLow = false;
      // Make sure device is connected
      if(!client.connected()) {
        connect();
      }            
      client.publish(outTopic, "CLOSED");
      currentState = "CLOSED";
      delay(50);
    }         
    takeLowTime = true;
  }

  if(digitalRead(sensorPin) == LOW)
  {
    //Serial.println("is Low");       
    if(takeLowTime)
    {
      lowIn = millis();          //save the time of the transition from high to LOW
      takeLowTime = false;       //make sure this is only done at the start of a LOW phase
    }
    //if the sensor is low for more than the given pause, 
    //we assume that no more detection is going to happen
    if(!lockLow && millis() - lowIn > pause)
    {  
      //makes sure this block of code is only executed again after 
      //a new detection sequence has been detected
      lockLow = true;
      // Make sure device is connected
      if(!client.connected()){
        connect();
      }
      currentState = "OPEN";                       
      client.publish(outTopic, "OPEN");
      delay(50);
    }
  }

}

void messageReceived(String &topic, String &payload) 
{
  //This sensor does not recieve anything from MQTT Server so this is blank
}
