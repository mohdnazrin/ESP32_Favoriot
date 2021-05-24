//############################################################################
//####  This program consist of:                                          #### 
//####  ESP32 (DOIT ESP32 DEVKIT V1) or ESP82666 (NodeMCU 1.0 ESP-12E Module)# 
//####  DHT11 Temperature & Humidity sensor                               #### 
//####  Relay 5V                                                          #### 
//####  Favoriot Iot Platform                                             ####
//####  Telegram Notification                                             ####
//############################################################################
//###########            Pin assignment for ESP32 board    ###################                                                     
//#### 18         DHT11                                                   ####
//#### 12         Relay pin for LAMP                                      ####
//#### 13         Relay pin for FAN                                       ####
//############################################################################
//###########            Pin assignment for ESP82666 board    ################                                                     
//#### D6         DHT11                                                   ####
//#### D7         Relay pin for LAMP                                      ####
//#### D8         Relay pin for FAN                                       ####

//######### Wifi Library #####################################################// 
#ifdef ESP32
  #include <WiFi.h>//ESP32
#else
  #include <ESP8266WiFi.h>//NodeMCU
#endif
#include <WiFiClient.h>
WiFiClient client;

//############      Json Library       #######################################//
#define ARDUINOJSON_USE_LONG_LONG 1 
#include <ArduinoJson.h>//ArduinoJson 6.17.3

//######### DHT11 Library ####################################################//
#include <DHT.h> //DHT Sensor Library by adafruit ver 1.2.3

//######### DHT11 Pins assignment (3 pins) ###################################//
#ifdef ESP32
  #define DHTPIN 18//middle pin of sensor connected to ESP32
#else
  #define DHTPIN D6//middle pin of sensor connected to NodeMCU
#endif
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//############# Relay Pin ####################################################//
#ifdef ESP32
  #define IN1  12  //LAMP
  #define IN2  13  //FAN
#else //ESP8266
  #define IN1  D7  //LAMP
  #define IN2  D8  //FAN
#endif

//########## Replace with your SSID and Password  ############################//
const char* ssid     = ""; // Add SSID
const char* password = ""; // add password

//########## Favoriot configuration ##########################################//
const String myDevice=""; //add your device
String apikey="";// add your APIkey
char server[]="apiv2.favoriot.com";

//########## Telegram configuration ##########################################//
#include <UniversalTelegramBot.h>
#define BOTtoken ""  
#define CHAT_ID ""
UniversalTelegramBot bot(BOTtoken, client);


void setup() {
  //Build in LED as output
  pinMode(LED_BUILTIN, OUTPUT);   
  
  //Serial COM  baud rate
  Serial.begin(115200); 

  //for relay pins
  pinMode(IN1, OUTPUT); //LAMP
  pinMode(IN2, OUTPUT); //FAN

  //initialize WiFi connection
  initWifi();

}

void loop() {
  // put your main code here, to run repeatedly:
  int counter;
   for (int counter=0; counter <= 5; counter++) {
       mainprogram();
       writedelay();}
  ESP.restart();//to restart ESP32
}

void mainprogram(){

  //read status actuator from cloud
  int LAMP,FAN;
  getDatafromFavoriot(&LAMP,&FAN); 

  //to remove uncertainty lamp state from cloud during initialization 
  if (LAMP>=1){
    LAMP=1;
  }
  else {
    LAMP=0;
  }

  //call DHT11 sensor
  float temp;
  float hum;
  read_DTH11(&temp,&hum);
    
  float temp_threshold;
  temp_threshold=25.5;  
    if (temp >= temp_threshold) {  
      digitalWrite(IN2, HIGH);
      Serial.println("\nFAN ON"); 
      FAN=1;           
                     }
    else {
      digitalWrite(IN2, LOW);
      Serial.println("\nFAN OFF");
      FAN=0; 
          } 

    if (LAMP == 0) { 
      digitalWrite(IN1, LOW);
      Serial.println("LAMP OFF");
                    }
    else {
      digitalWrite(IN1, HIGH);
      Serial.println("LAMP ON");
          }    

  // Turn the on-board LED blue on
  digitalWrite(LED_BUILTIN, HIGH);
  
  //send all data to favoriot
  String json = "{\"device_developer_id\":\""+ myDevice + "\",\"data\":{\"1-temperature\":\""+String(temp)+"\",\"2-humidity\":\""+String(hum) +"\",\"3-lamp\":\""+String(LAMP) +"\",\"4-fan\":\""+String(FAN) +"\" }}";
  sendDatatoFavoriot(json);    
  Serial.println(json);

  // Turn theon-board LED blue off
  digitalWrite(LED_BUILTIN, LOW);

//############################################################################
//##### The segment below is code to notify a user via Telegram  #############
//############################################################################
  float temp_limit_min=20;
  float temp_limit_max=35;
  if (temp < temp_limit_min) {  
      bot.sendMessage(CHAT_ID, "Alert: Temperature is too low!!", ""); 
      Serial.println("\nTELEGRAM SEND");           
                     }
  else if (temp > temp_limit_max) {  
      bot.sendMessage(CHAT_ID, "Alert: Temperature is too hot!!", ""); 
      Serial.println("\nTELEGRAM SEND");           
                     }
                     
}

//############################################################################
//##### The segment below is code for individual sensors #####################
//############################################################################

//##### Read DHT11 Sensor ####################################################
void read_DTH11(float *temp, float *hum)
{
  read_DHT11:
    //float temp=0.0, hum=0.0;    
    int check=dht.read(DHTPIN);
    *temp=dht.readTemperature();
    if (isnan(*temp))  //if nan value, read again
      { goto read_DHT11;  }
    *hum=dht.readHumidity();
        if (isnan(*hum))
      { goto read_DHT11;  }
    Serial.println("Air Temperature (C):");
    Serial.println(*temp);
    Serial.println("Air Humidity (%):");
    Serial.println(*hum); 
    return ;   
}

//##### Establish a Wi-Fi connection with your router   #######################
void initWifi() {
  Serial.print("Connecting to: "); 
  Serial.print(ssid);
  WiFi.begin(ssid, password);  

  int timeout = 10 * 4; // 10 seconds
  while(WiFi.status() != WL_CONNECTED  && (timeout-- > 0)) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");

  if(WiFi.status() != WL_CONNECTED) {
     Serial.println("Failed to connect, going back to sleep");
  }

  Serial.print("WiFi connected in: "); 
  Serial.print(millis());
  Serial.print(", IP address: "); 
  Serial.println(WiFi.localIP());
}

//##### Delay 10s   ##########################################################
void writedelay(){
  int secound, minute;
  secound=10;
  minute=1;
  delay(1000*secound*minute);
}

//############################################################################
//##### The segment below is code to read data from Cloud ####################
//############################################################################

void getDatafromFavoriot(int *LAMP,int *FAN){
  if (client.connect(server,80)){
    Serial.println("STATUS:Getting data...");    
    client.println("GET /v2/streams?device_developer_id=name@yourdeveloperID&max=1 HTTP/1.1");
    client.println("HOST: apiv2.favoriot.com");
    client.print(String("apikey:"));
    client.println(apikey);
    client.println("Accept:application/json");
    client.println("Content-Type:application/json");
    client.println("cache-control:no_cache");
    client.println("Connection:close");
    client.println();
    Serial.println("STATUS: Data retrieve !");  

  }
    else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
          }
     
/////to get string that read from favoriot to display on Serial Monitor  
while (client.connected()) {//to filter only json string
    String input = "";
    bool begin = false;
      while (client.available() || !begin) {
      char c = client.read();
      if (c == '{') {
        begin = true;
      }
      if (begin) input += (c);
      if (c == '"}') {
        break;
      }
      delay(1);          
         }
      Serial.println(input); //copy this response from Serial Monitor to paste on following steps   

  //This is the Assistant for ArduinoJson 6.17.3. Make sure the same version is installed on your computer.
  //refer to https://arduinojson.org/v6/assistant/ to define bufferSize and following code

//############################################################################
//####### Paste code generated from JsonAssistant below ######################


//############################################################################
//############ Read the register to specific data ############################

  *LAMP = 0;
  *FAN =  0;       
   } 
  if (!client.connected()){
    client.stop();
  }  

}

//############################################################################
//##### The segment below is code to write data to Cloud #####################
//############################################################################

void sendDatatoFavoriot(String json){
  
  if (client.connect(server,80)){
    Serial.println("STATUS:Sending data...");
    client.println("POST /v2/streams HTTP/1.1");    
    client.println("HOST: apiv2.favoriot.com");
    client.print(String("apikey:"));
    client.println(apikey);
    client.println("Content-Type:application/json");
    client.println("cache-control:no_cache");
    client.print("Content-Length:");
    int thisLength=json.length();
    client.println(thisLength);
    client.println("Connection:close");
    client.println();
    client.println(json);
    Serial.println("STATUS: Data sent !");    
  }
  if (!client.connected()){
    client.stop();
  }
}
