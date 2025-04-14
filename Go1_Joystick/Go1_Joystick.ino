#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// the default password for a Unitree Go1 AP
#define PASSWORD "00000000"

// decide here if you run the esp wi5V or 3,3V. Values of analog converter differ
#define _5VOLT false

// mqtt stuff
const char* mqtt_server = "192.168.12.1";  // you can also call the ip of the raspi (192.168.123.161)
const int mqtt_port = 1883;
const char* mqtt_client_id = "00";
const char* mqtt_topic_stick = "controller/stick";  // we simulate the normal controller.
const char* mqtt_topic_command = "controller/action";  // the action topic for the bot.
const char* mqtt_topic_debug_xy = "debug/xy";  // you may observer debug/xy with an mqtt client like MQTTX to adjust joystick values.
const char* command_walk = "walk";  // we start with just walking.

Adafruit_ADS1115 ads;

WiFiClient espClient;
PubSubClient client(espClient);


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      sendData(mqtt_topic_command, (char *)command_walk, sizeof(command_walk));
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void sendData(const char* mqtt_topic, float* data, size_t size) {
  if (!client.publish(mqtt_topic, (uint8_t*)data, size)) {
    Serial.println("Error sending data");
  } else {
    Serial.println("Sending data succesful");
  }
}

void sendData(const char* mqtt_topic, char* data, size_t size) {
  if (!client.publish(mqtt_topic, (uint8_t*)data, size)) {
    Serial.println("Error sending data");
  } else {
    Serial.println("Sending data succesful");
  }
}


void setup(void)
{
    Serial.begin(115200);
    Serial.println("Go1 Joystick 1.0");
     
    Serial.println("Getting single-ended readings from I2C AIN0,AIN1");
    ads.setGain(GAIN_TWOTHIRDS); //set to +- 6144 mV
    Serial.println("ADC Range set to: +/- 6144 mV (ADS1115: 1 bit = 0.192 mV");
    ads.begin();

    Serial.println("Set WiFi to station mode and disconnect from an AP if it was previously connected");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("Wifi Setup done");

    Serial.println("Wifi scan start");

    bool bGo1 = false;
    String sGo1SSID = "";

    // try 5 times to get the SSID from a network list. We will do that until we find a unitree go1
    for (int j = 0; j < 5; j++) {
        // WiFi.scanNetworks will return the number of networks found
        int n = WiFi.scanNetworks();
        Serial.println("scan done");
        if (n == 0) {
            Serial.println("no networks found");
        } else {
            Serial.print(n);
            Serial.println(" networks found");
            for (int i = 0; i < n; ++i) {
                // Print SSID and RSSI for each network found
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(WiFi.SSID(i));
                int aIndex = WiFi.SSID(i).indexOf("Unitree_Go");
                Serial.print(" IndexOf: ");
                Serial.println(aIndex);
                // if we found a Go1 we can leave the for-loop. Let's store the SSID for a later connect as a client
                if(WiFi.SSID(i).indexOf("Unitree_Go") == 0){
                    Serial.println("Found a Go1");
                    bGo1 = true;
                    sGo1SSID = WiFi.SSID(i);
                    break;
                }
                delay(10);
            }
            // if we found a Go1 we can leave the for-loop
            if (bGo1) {
              break;
            }
        }
        Serial.println("");
    }

    // the scan was not succesful? Lets restart the ESP and try it again. Without a Go1 this makes no sense
    if (bGo1 == false) {
        Serial.println("restarting");
        delay(2000);
        ESP.restart();
    }

    // here we found a Go1. Let's connect to the stored SSID
    WiFi.mode(WIFI_STA);
    WiFi.begin(sGo1SSID, PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(250);
    }

    Serial.println("");

    Serial.println("Connected to " + sGo1SSID + " with mac:" + WiFi.macAddress());

    client.setServer(mqtt_server, mqtt_port);
}

float last_x = 0.00;
float last_y = 0.00;
uint iCommand = 0;
float x = 0.0;
float y = 0.0;
// handle the analog joystick values transported over i2c 
int16_t adc0, adc1;
char myDebugString[50];
 
void loop(void)
{
    // connect/reconnect the mqtt client
    if (!client.connected()) {
      reconnect();
    }

    adc0 = ads.readADC_SingleEnded(0);
    adc1 = ads.readADC_SingleEnded(1);
 
    if ( _5VOLT ) {
      // with 5V
      x = (adc0-8730)/8900.0;
      y = (adc1-8812)/8900.0;
    } else {
      // with 3,7V
      x = (adc0-10350)/10600.0;
      y = (adc1-10450)/10600.0;
    }

    // you may fetch these debug data with an mqtt client also connected to the bot.
    sprintf(myDebugString, "adc0/1: %d %d \r\n   x/y: %.3f %.3f", adc0, adc1, x, y);
    sendData(mqtt_topic_debug_xy, myDebugString, strlen(myDebugString));

    Serial.print("Curr X: ");
    Serial.print(x);
    Serial.print(" Curr Y: ");
    Serial.println(y);

    Serial.print("Last X: ");
    Serial.print(last_x);
    Serial.print(" Last Y: ");
    Serial.println(last_y);
    
    // check, if we got a command, but only after the joystick was centered
    if ((fabs(last_x) < 0.03) && (fabs(last_y) < 0.03)) {
      Serial.println("JoyStick was centered");
      // release command mode, when joystick is released;
      iCommand = 0;
      // lets check, if this is a fast push of the joystick
      if ((y - last_y) > 0.7) {
        Serial.println("command FORWARD");
        iCommand = 1;
      }
      if ((y - last_y) < -0.7) {
        Serial.println("command BACKWARD");
        iCommand = 2;
      }
      if ((x - last_x) > 0.7) {
        Serial.println("command RIGHT");
        iCommand = 3;
      }
      if ((x - last_x) < -0.7) {
        Serial.println("command LEFT");
        iCommand = 4;
      }
    }

    if (iCommand > 0) {
      Serial.println("Sending command"); 
      // in work
    } else {
      Serial.println("Sending joystick values");
      // now handle mqtt
      float c[4];
      c[0] = 0.0;
      c[1] = x;
      c[2] = 0.0;
      c[3] = y;
  
      sendData(mqtt_topic_stick, c, sizeof(c));
    }

    last_x = x;
    last_y = y;
    
    delay(200);
}
