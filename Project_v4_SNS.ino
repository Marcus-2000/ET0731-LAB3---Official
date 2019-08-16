#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Arduino.h> 
#include "ESP32_MailClient.h"

#define LED_PIN 2

// Defining the topics for the device to subscribe and publish to
#define aws_iot_sub_topic "fire-alarm/topic"
#define aws_iot_pub_topic "fire-alarm/fire_detected"

/* Change the following configuration options */
const char* ssid = "ssid"; // SSID of WLAN
const char* password = "pass";// password for WLAN access
const char* aws_iot_hostname = "a2hhmjt6d62qg5-ats.iot.us-east-1.amazonaws.com"; // Change Host name
// const char* aws_iot_sub_topic = "topic/hello"; //Change Subscription Topic name here
// const char* aws_iot_pub_topic = "another/topic/echo"; // Change publish topic name here
const char* aws_iot_pub_message = "Test.";
const char* aws_iot_pub_message2 = "There is a fire detected.";
const char* client_id = "MyIoT";

// change the ca certificate
const char* ca_certificate = "-----BEGIN CERTIFICATE-----\n \n-----END CERTIFICATE-----";
// change the IOT device certificate
const char* iot_certificate = "-----BEGIN CERTIFICATE-----\n \n-----END CERTIFICATE-----\n"; //Own certificate
// change to the private key of aws
const char* iot_privatekey = "-----BEGIN RSA PRIVATE KEY-----\n \n-----END RSA PRIVATE KEY-----\n";// RSA Private Key

//WiFi or HTTP client for internet connection
HTTPClientESP32Ex http;

//The Email Sending data object contains config and data to send
SMTPData smtpData;

//Callback function to get the Email sending status
void sendCallback(SendStatus info);

#define SSID_HAS_PASSWORD //comment this line if your SSID does not have a password
/*Variables for sensor*/
int SmokeSensor = 34;
int SmokeSensorVal = 0;
int sensorThres = 300; //Smoke sensor threshold
int DataToSent = 0;

// Temperature
int TempSensor = 32;
float TempSensorVal = 0;
float Temp = 0;
int TempThres = 75;

// Buzzer
int buzzerPin = 14;

int j = 0;

/* Global Variables */
WiFiClientSecure client;
PubSubClient mqtt(client);

/* Functions */
void sub_callback(const char* topic, byte* payload, unsigned int length) {
  Serial.print("Topic: ");
  Serial.println(topic);

  Serial.print("Message: ");
  for (int i = 0; i < length; i++)
    Serial.print((char) payload[i]);
  Serial.println();

  if ((char) payload[0] == '1')
    digitalWrite(LED_PIN, HIGH);
  else if ((char) payload[0] == '0')
    digitalWrite(LED_PIN, LOW);

  mqtt.publish(aws_iot_pub_topic, aws_iot_pub_message);
}

void setup() {
  //Initializations
  Serial.begin(115200);
  Serial.print("Attempting WiFi connection on SSID: ");
  Serial.print(ssid);
  pinMode(LED_PIN, OUTPUT);
  ledcSetup(0,1E5,12);
  ledcAttachPin(14,0);
  digitalWrite(LED_PIN, LOW);

  WiFi.localIP();
  Serial.println(WiFi.localIP());
  
  // WiFi
  #ifdef SSID_HAS_PASSWORD
  WiFi.begin(ssid, password);
  #else
  WiFi.begin(ssid);
  #endif

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.print("\nWiFi connection succeeded.\n");

  client.setCACert(ca_certificate);
  client.setCertificate(iot_certificate);
  client.setPrivateKey(iot_privatekey);

  // AWS IoT MQTT uses port 8883
  mqtt.setServer(aws_iot_hostname, 8883);
  mqtt.setCallback(sub_callback);

  
}

void loop() {
  // reconnect on disconnect
  while (!mqtt.connected()) {
    Serial.print("Now connecting to AWS IoT: ");
    if (mqtt.connect(client_id)) {
      Serial.println("connected!");

      const char message3[25] = "{\"fire\": 1}";
      const char message4[25] = "{\"fire\": 0}";
      
      while(1) {         // while loop to repeatedly send data to AWS
        //Smoke detector portion
        SmokeSensorVal = analogRead(SmokeSensor);
        
        //LM35 portion
        TempSensorVal = analogRead(TempSensor);
        
        Temp = (100 * TempSensorVal) / 1023;

        Serial.print("Temperature is ");
        Serial.println(Temp);
        Serial.print("Smoke is ");
        Serial.println(SmokeSensorVal);

        
        if ((SmokeSensorVal > sensorThres) || (Temp > TempThres)) {
          j++;
        } else {
          j = 0;
        }

        if (j == 1) {
          mqtt.publish(aws_iot_pub_topic, message3); // Sending message to specific topic 
          ledcWriteTone(0,200);
          delay(5000);
        } else if(j == 0) {
          mqtt.publish(aws_iot_pub_topic, message4); // Sending message to specific topic
          ledcWriteTone(0,0);
        }

        delay(20000);
    }
  } else {
      Serial.print("failed with status code ");
      Serial.print(mqtt.state());
      Serial.println(" trying again in 5 seconds...");
      delay(5000);

  mqtt.loop();
}
  }
}
