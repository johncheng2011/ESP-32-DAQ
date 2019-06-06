#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <stdlib.h>
#include <pthread.h>
#include "WiFi.h"
#include "esp_wps.h"
#define ESP_WPS_MODE      WPS_TYPE_PBC
#define ESP_MANUFACTURER  "ESPRESSIF"
#define ESP_MODEL_NUMBER  "ESP32"
#define ESP_MODEL_NAME    "ESPRESSIF IOT"
#define ESP_DEVICE_NAME   "ESP STATION"


const int buttonPin = 12;

//pin defninitons
const int digital_in_one = 4;
const int digital_in_two = 39;
const int digtial_in_three = 34;
const int digital_in_four = 35;

const int digital_out_one = 36;
const int digital_out_two = 33;
const int digital_out_three = 25;
const int digital_out_four = 26;

const int analog_in_one = 36;
const int analog_in_two = 0;
const int analog_in_three = 0;
const int analog_in_four = 0;

int buttonState;
int lastButtonState= LOW;
int status = WL_IDLE_STATUS;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

static esp_wps_config_t config;

void wpsInitConfig(){
  config.crypto_funcs = &g_wifi_default_wps_crypto_funcs;
  config.wps_type = ESP_WPS_MODE;
  strcpy(config.factory_info.manufacturer, ESP_MANUFACTURER);
  strcpy(config.factory_info.model_number, ESP_MODEL_NUMBER);
  strcpy(config.factory_info.model_name, ESP_MODEL_NAME);
  strcpy(config.factory_info.device_name, ESP_DEVICE_NAME);
}

String wpspin2string(uint8_t a[]){
  char wps_pin[9];
  for(int i=0;i<8;i++){
    wps_pin[i] = a[i];
  }
  wps_pin[8] = '\0';
  return (String)wps_pin;
}
void connectAWSIoT();
void mqttCallback (char* topic, byte* payload, unsigned int length);

void WiFiEvent(WiFiEvent_t event, system_event_info_t info){

pinMode(LED_BUILTIN, OUTPUT);
  
  switch(event){
    case SYSTEM_EVENT_STA_START:
      Serial.println("Station Mode Started");
      digitalWrite(LED_BUILTIN, LOW);
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("Connected to :" + String(WiFi.SSID()));
      Serial.print("Got IP: ");
      Serial.println(WiFi.localIP());
      status = WL_CONNECTED;
      digitalWrite(LED_BUILTIN, HIGH);
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("Disconnected from station, attempting reconnection");
      WiFi.reconnect();
      digitalWrite(LED_BUILTIN, LOW);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      Serial.println("WPS Successfull, stopping WPS and connecting to: " + String(WiFi.SSID()));
      esp_wifi_wps_disable();
      delay(10);
      WiFi.begin();
      digitalWrite(LED_BUILTIN, LOW);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      Serial.println("WPS Failed, retrying");
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&config);
      esp_wifi_wps_start(0);
      digitalWrite(LED_BUILTIN, LOW);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      Serial.println("WPS Timedout, retrying");
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&config);
      esp_wifi_wps_start(0);
      digitalWrite(LED_BUILTIN, LOW);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      Serial.println("WPS_PIN = " + wpspin2string(info.sta_er_pin.pin_code));
      break;
    default:
      break;
  }
}


const char *endpoint = "aos5w3e21mang-ats.iot.us-east-2.amazonaws.com";
// Example: xxxxxxxxxxxxxx.iot.ap-northeast-1.amazonaws.co
const int port = 8883;
char *pubTopic = "$aws/things/IotThing/shadow/update";
char *subTopic = "$aws/things/IotThing/shadow/update/delta"; // to send

const char* rootCA = \
"-----BEGIN CERTIFICATE-----\n" 
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
"rqXRfboQnoZsG4q5WTP468SQvvG5\n" 
"-----END CERTIFICATE-----\n";

const char* certificate = "-----BEGIN CERTIFICATE-----\n"
"MIIDWjCCAkKgAwIBAgIVAIUd27etxXxhqrycjiw5mxnKIRpMMA0GCSqGSIb3DQEB\n"
"CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\n"
"IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0xOTAzMDQyMzA5\n"
"MDVaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\n"
"dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDBnqoQciwT1FzT3lAe\n"
"NNTSiqArBhEC3bXtInj3dQGyJ12UYv3kdqyUHaTEplzWQuu6PcwE/Tdn4aY+/7NZ\n"
"OLfjQwB6EutkBUQvtcFs0RR0b6crU7kJp0uBdYo4HECqKLtvs4TvXlUx9YMqnaB7\n"
"1EBqRhYKw2GPwtZaUAaVE5bbNbbzqN+eQM4ZBzAOklUfGdqEEGYUb0c/uDt/vMmk\n"
"Z+CUKkiDOuyddJaOggU0F9k2rSL837+0VGiaoNNZxegt1DDaxA78JJWKeL6u20m2\n"
"gohHCN7pKO9ox1kO/7rLA0Lohi/o/PnHwq5m8lvhcwo89nhtJv7mPfQ9ki4vGgA3\n"
"Xw/XAgMBAAGjYDBeMB8GA1UdIwQYMBaAFDAJM8jooWzm7ix4Qkbbkg6m6cC9MB0G\n"
"A1UdDgQWBBRtrduHZgRXtYnBa2p/Q+f/IPgy8TAMBgNVHRMBAf8EAjAAMA4GA1Ud\n"
"DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAMtkaFJqtm1cdxb1hCEuixC/g\n"
"3WUQf6rbG3i84fptDjET5h3UJ0DjEokCz3AeSj0XTqTPCxS9t2BlC+MvQ0x9Vvq9\n"
"zEayC6J/vaODcOdp/H/YLycWDdonntp3YCvta9Pje3tinOpzOyUEGyJKWD/UOtlU\n"
"Z9RQ16rOzaXY9vcH1ZZaR4ziO8gAop3Zx+pJ/5Cd2NmaTw2V9NrQtlvkMYddjCOo\n"
"ApaEiUK90LBZQOYYD/ylkxQIrQJpl1Wk6wVE48QuyxepLhXMy8XdpmfDfy7G3ndK\n"
"1iaSSeusMVTHfLLhmOSHffcpykhY5iReKjFbshHqrtAuK2XuOqo+pD7KcCk5Uw==\n" 
"-----END CERTIFICATE-----\n";

const char* privateKey = "-----BEGIN RSA PRIVATE KEY-----\n" 
"MIIEowIBAAKCAQEAwZ6qEHIsE9Rc095QHjTU0oqgKwYRAt217SJ493UBsiddlGL9\n"
"5HaslB2kxKZc1kLruj3MBP03Z+GmPv+zWTi340MAehLrZAVEL7XBbNEUdG+nK1O5\n"
"CadLgXWKOBxAqii7b7OE715VMfWDKp2ge9RAakYWCsNhj8LWWlAGlROW2zW286jf\n"
"nkDOGQcwDpJVHxnahBBmFG9HP7g7f7zJpGfglCpIgzrsnXSWjoIFNBfZNq0i/N+/\n"
"tFRomqDTWcXoLdQw2sQO/CSVini+rttJtoKIRwje6SjvaMdZDv+6ywNC6IYv6Pz5\n"
"x8KuZvJb4XMKPPZ4bSb+5j30PZIuLxoAN18P1wIDAQABAoIBACVg6Bv8rp21ZaZR\n"
"SM5MDLoIoRstNKOFAdYhzZCYOheWme5HnhQ5BOAjSOfd5hZHHRL7UyOzbrrTSTDo\n"
"VEtpM7bf/HuUo3TPv3YmdIz5YCWKWI94vkQq09zOQDzM138CFg1ebs93OqNGbmgP\n"
"vieptjXOCftR9Me4KYGN6XdmLL1srpHTb6DK9oPe1/gct+JrTgDO7b/RZ/TdMMuD\n"
"LsSNoiJHsgnnSNE1GJGrXpV035ZNEqYkXDYbkuUIkIngYbHbDCuCmG8KvPODtvXF\n"
"hl42aQdq0RZvmSfwkfRK9T4ic1CqZCC+B0kFH33gQ1rAdoofLGchRFvDbMHjcXzN\n"
"T+5E3qECgYEA/XNu/sIO2V2Tg3yBvOdPy5C4HJ7gh4tTp7UEKBLl4iOaD4qpwaiN\n"
"Zp3v3cZLMGrecrNNqOv742ZktFphPodhuc2tkaXxdy8Vs9fJIxxtYYkktjUrLqia\n"
"JuK6scQKO+kNsVVFbIED8q9RssXQMaiP/lKs9AM+lr7KtbIhnN8rxuMCgYEAw5Eu\n"
"mU7o4ouXXrs/S21inScLQ+ZmNF5BA06lpWCyA/HxVs3OYZQQq+Cgt6PLbJi5OK4c\n"
"7V/cqxHZGtIM79tx8D3BWKPAwaPPm69nDLUuLQSNJIzN7Zk2JDwIoEYwmdVZet8b\n"
"BXw8QR6p8XwIzIyDzjrl8Lk9MIydekOHtehUsX0CgYAEMTCYziTou3+BIUIUGc9c\n"
"Epy4/HfCRi+wCnJzJzzeLLCTqTt9lIgiNmKNTIZZ2qqrQwP5001rSXpI8WXCXwLi\n"
"y+AfFJuV2RWGz/7nscStZFNTIDYCo49JLV7hKdjxfL1ZPYvUa53hGb1EM7lp2Nvo\n"
"3P03XAZg/+7ianvb2GUk+QKBgAW9BdQDx7uWAvwJnILXE9Sup5r3cLpKpbe2IBAp\n"
"NS/+cmlsooikcpTIg46/5KEsHgs2uKySaoQuguNRlIWZN8+n0DuMmoRUDSxxSiHK\n"
"Uy+I+ac/5m75VuhbRpmFVbm90+FSGJXhnlb+0le+nvd5jAiKG+MjXFNHXPuQbbP9\n"
"vDZ1AoGBAMbhB9iiIpdKkmEPZvf+474w5PU7BfQF0BZQlz6sbK/PaP5T8MKkTVWm\n"
"a3x5huLn3Sx1RAb74J/ClbyOAabWUKTjoaBQm3w0XOCPCu0TX93IetFEhZBAq263\n"
"CZ7ntTvwZTFYMrNNMyGCXW1sYVFTE3tMoY3QWioU2IdB+njBkJr0\n" 
"-----END RSA PRIVATE KEY-----\n";

WiFiClientSecure httpsClient;
PubSubClient mqttClient(httpsClient);

char* ssid = "Capstone49";
char* password = "capS49%2O19";
void setup() {

  pinMode(buttonPin,INPUT);
  
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(digital_in_one,INPUT);
  pinMode(digital_in_two,INPUT);
  pinMode(digtial_in_three,INPUT);
  pinMode(digital_in_four,INPUT);

  pinMode(digital_out_one,OUTPUT);
  pinMode(digital_out_two,OUTPUT);
  pinMode(digital_out_three,OUTPUT);
  pinMode(digital_out_four,OUTPUT);

  pinMode(analog_in_one,INPUT);
  pinMode(analog_in_two,INPUT);
  pinMode(analog_in_three,INPUT);
  pinMode(analog_in_four,INPUT);
   
  Serial.println("setup");
  Serial.begin(115200);
/*
// Start WiFi
    while(1){
      if(digitalRead(12)){
        break;
      }
    }
    Serial.println("Connecting to ");
    Serial.print(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    digitalWrite(LED_BUILTIN,HIGH);
    Serial.println("\nConnected.");
    pthread_t tid;
    int i = 0;
    pthread_create(&tid,NULL,mqttLooper,(void*)i);
  */
  
  while(status != WL_CONNECTED){
   int reading = digitalRead(buttonPin);
   if( reading == HIGH){
        Serial.println("press");
        WiFi.onEvent(WiFiEvent);
        WiFi.mode(WIFI_MODE_STA);

        Serial.println("Starting WPS");

        wpsInitConfig();
        esp_wifi_wps_enable(&config);
        esp_wifi_wps_start(0);
  }
  }
  
  

    // Configure MQTT Client
    httpsClient.setCACert(rootCA);
    httpsClient.setCertificate(certificate);
    httpsClient.setPrivateKey(privateKey);
    mqttClient.setServer(endpoint, port);
    mqttClient.setCallback(mqttCallback);

    connectAWSIoT();
    
}

void connectAWSIoT() {
    while (!mqttClient.connected()) {
        if (mqttClient.connect("ESP32_device")) {
            Serial.println("Connected.");
            int qos = 0;
            mqttClient.subscribe(subTopic, qos);
            Serial.println("Subscribed.");
        } else {
            Serial.print("Failed. Error state=");
            Serial.print(mqttClient.state());
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

long messageSentAt = 0;
//int dummyValue = 0;
char pubMessage[128];

void mqttCallback (char* topic, byte* payload, unsigned int length) {
    Serial.print("Received. topic=");
    Serial.println(topic);
    
    
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.print("\n");
    if(length == 2){
      //get from message
      char temp_pin = (char)payload[0];
      char temp_value = (char)payload[1];
      int pin = (int)(temp_pin - '0');
      int value = (int)(temp_value - '0');

      Serial.println(pin);
      Serial.println(value);
      switch(pin){
        case 1:
          Serial.println("one out");
          writeDigitalOut(digital_out_one,value);
          break;
        case 2:
        Serial.println("one two");
          writeDigitalOut(digital_out_two,value);
          break;
        case 3:
        Serial.println("one three");
          writeDigitalOut(digital_out_three,value);
          break;
        case 4:
        Serial.println("one four");
          writeDigitalOut(digital_out_four,value);
          break;
        default:
          Serial.print("invalid input\n");
        
      }
    }
}

void writeDigitalOut( int pinNum, int val){
  Serial.print("Writing ");
  Serial.print(pinNum);
  Serial.println(val);
  digitalWrite(pinNum,val);
  
}

void sendDigitalRead(int pinNum){
  int numRead = digitalRead(pinNum);
  sprintf(pubMessage, "%d %d", pinNum, numRead);
  Serial.print("Publishing message to topic ");
  Serial.println(pubTopic);
  Serial.println(pubMessage);
  mqttClient.publish(pubTopic, pubMessage);
  Serial.println("Published.");
  
}

void sendAnalogRead(int pinNum){
  int numRead = analogRead(pinNum);
  sprintf(pubMessage, "%d %d", pinNum, numRead);
  Serial.print("Publishing message to topic ");
  Serial.println(pubTopic);
  Serial.println(pubMessage);
  mqttClient.publish(pubTopic, pubMessage);
  Serial.println("Published.");
  
}
void* mqttLooper(void* ii){
  while(1){
    mqttClient.loop();
  }
}

void mqttLoop() {
    if (!mqttClient.connected()) {
        connectAWSIoT();
    }

sleep(.5);
      sendDigitalRead(digital_in_one);
      sendDigitalRead(digital_in_two);
      sendDigitalRead(digtial_in_three);
      sendDigitalRead(digital_in_four);
      sendAnalogRead(analog_in_one);
sleep(.5);

      
      //sendAnalogRead(analog_in_two);
      //sendAnalogRead(analog_in_three);
      //sendAnalogRead(analog_in_four);
      
     
    

    
    /*
    if(int one = digitalRead(14)){
      if (now - messageSentAt > 500) {
          messageSentAt = now;
          //sprintf(pubMessage, "Hello, people", dummyValue++);
          sprintf(pubMessage, "%d %d", one, digital_in_one);
          Serial.print("Publishing message to topic ");
          Serial.println(pubTopic);
          Serial.println(pubMessage);
          mqttClient.publish(pubTopic, pubMessage);
          Serial.println("Published.");
      }
    
    }
    */
}

void loop() {
  mqttLoop();
}
