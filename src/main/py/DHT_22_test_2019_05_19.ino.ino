#include "DHTesp.h"
#include "Ticker.h"
#include <WiFiClientSecure.h>

#ifndef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP32 ONLY!)
#error Select ESP32 board.
#endif

//home BA
//const char* ssid     = "HUAWEI-5C86-2G"; //2G
//const char* password = "pX4f9Bv9";
const char* ssid     = "HUAWEI-iot-2G";  // iot
const char* password = "ucyhp70333";

// radosina
//const char* ssid     = "ZTE-9DS4ZJ";
//const char* password = "dddb6ax27rkx";

String riadok;
float teplota_namerana;

String polo_obyvacka = "polo_obyvacka";
String polo_mediabox = "polo_mediabox";
String polo_detska = "polo_detska";

String miestnost = polo_obyvacka;
/**************************************************************/
/* Example how to read DHT sensors from an ESP32 using multi- */
/* tasking.                                                   */
/* This example depends on the ESP32Ticker library to wake up */
/* the task every 20 seconds                                  */
/* Please install Ticker-esp32 library first                  */
/* bertmelis/Ticker-esp32                                     */
/* https://github.com/bertmelis/Ticker-esp32                  */
/**************************************************************/

DHTesp dht;
DHTesp dht2;
DHTesp dht3;
DHTesp dht4;
DHTesp dht5;

/** Pin number for DHT11 data pin */
int dhtPin = 13;
int dhtPin2 = 14;
int dhtPin3 = 26;
int dhtPin4 = 25;
int dhtPin5 = 32;

/** Comfort profile */
ComfortState cf;

void tempTask(void *pvParameters);
bool getTemperature();
bool getTemperatureOneDHT(DHTesp dht);
void triggerGetTemp();

/** Task handle for the light value read task */
TaskHandle_t tempTaskHandle = NULL;
/** Ticker for temperature reading */
Ticker tempTicker;

/** Flag if task should run */
bool tasksEnabled = false;


//prod time
int loopTimeInSecond = 600;

//debug time
//int loopTimeInSecond = 20;

/**
 * initTemp
 * Setup DHT library
 * Setup task and timer for repeated measurement
 * @return bool
 *    true if task and timer are started
 *    false if task or timer couldn't be started
 */
bool initTemp() {
  byte resultValue = 0;
  // Initialize temperature sensor
  dht.setup(dhtPin2, DHTesp::DHT22);
//  dht2.setup(dhtPin2, DHTesp::DHT22);
//  dht3.setup(dhtPin3, DHTesp::DHT22);
//  dht4.setup(dhtPin4, DHTesp::DHT22);
//  dht5.setup(dhtPin5, DHTesp::DHT22);
  
  Serial.println("DHT initiated");

  // Start task to get temperature
  xTaskCreatePinnedToCore(
      tempTask,                       /* Function to implement the task */
      "tempTask ",                    /* Name of the task */
      4000,                           /* Stack size in words */
      NULL,                           /* Task input parameter */
      5,                              /* Priority of the task */
      &tempTaskHandle,                /* Task handle. */
      1);                             /* Core where the task should run */

  if (tempTaskHandle == NULL) {
    Serial.println("Failed to start task for temperature update");
    return false;
  } else {
    // Start update of environment data every 20 seconds
    tempTicker.attach(loopTimeInSecond, triggerGetTemp);
  }
  return true;
}

/**
 * triggerGetTemp
 * Sets flag dhtUpdated to true for handling in loop()
 * called by Ticker getTempTimer
 */
void triggerGetTemp() {
  if (tempTaskHandle != NULL) {
     xTaskResumeFromISR(tempTaskHandle);
  }
}

/**
 * Task to reads temperature from DHT11 sensor
 * @param pvParameters
 *    pointer to task parameters
 */
void tempTask(void *pvParameters) {
  Serial.println("tempTask loop started");
  while (1) // tempTask loop
  {
    if (tasksEnabled) {
      // Get temperature values
      getTemperature();
    }
    // Got sleep again
    vTaskSuspend(NULL);
  }
}

/**
 * getTemperature
 * Reads temperature from DHT11 sensor
 * @return bool
 *    true if temperature could be aquired
 *    false if aquisition failed
*/
bool getTemperature() {
  // Reading temperature for humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  getTemperatureOneDHT(dht);
  //getTemperatureOneDHT(dht2);
  //getTemperatureOneDHT(dht3);
  //getTemperatureOneDHT(dht4);
  //getTemperatureOneDHT(dht5);
  
  return true;
}

bool getTemperatureOneDHT(DHTesp dht){
    // Reading temperature for humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  TempAndHumidity newValues = dht.getTempAndHumidity();
  // Check if any reads failed and exit early (to try again).
  if (dht.getStatus() != 0) {
    Serial.println("DHT11 error status: " + String(dht.getStatusString()) + dht.getModel());
    return false;
  }

  float heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
  float dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
  float cr = dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);
  String comfortStatus = comfortMapper(cf);
  Serial.println(" T:" + String(newValues.temperature) + " H:" + String(newValues.humidity) + " I:" + String(heatIndex) + " D:" + String(dewPoint) + " " + comfortStatus);

  logTemp(newValues.temperature, newValues.humidity);
  
  return true;
}


String comfortMapper(ComfortState cf3){
  String comfortStatus;
  switch(cf3) {
    case Comfort_OK:
      comfortStatus = "Comfort_OK";
      break;
    case Comfort_TooHot:
      comfortStatus = "Comfort_TooHot";
      break;
    case Comfort_TooCold:
      comfortStatus = "Comfort_TooCold";
      break;
    case Comfort_TooDry:
      comfortStatus = "Comfort_TooDry";
      break;
    case Comfort_TooHumid:
      comfortStatus = "Comfort_TooHumid";
      break;
    case Comfort_HotAndHumid:
      comfortStatus = "Comfort_HotAndHumid";
      break;
    case Comfort_HotAndDry:
      comfortStatus = "Comfort_HotAndDry";
      break;
    case Comfort_ColdAndHumid:
      comfortStatus = "Comfort_ColdAndHumid";
      break;
    case Comfort_ColdAndDry:
      comfortStatus = "Comfort_ColdAndDry";
      break;
    default:
      comfortStatus = "Unknown:";
      break;
  };
  return comfortStatus;

}

bool initWiFi(){
  // Pripoj sa na Wifi
  Serial.println();
  Serial.println();
  Serial.print("Pripajam na siet ");
  Serial.println(ssid);
  
  while(true) {
   
    startWifi();
       
    if( connectWifi() == false) {
      Serial.println("wifiInitialized: false");
      delay(20000);
      continue;
    } else {
      Serial.println("wifiInitialized: true"); 
      Serial.println("");
      Serial.println("WiFi pripojena");  
      Serial.println("IP adresa: ");
      Serial.println(WiFi.localIP());

      return true;
    }
  }
  return true;
}

boolean connectWifi() {
   int cycle = 50;
   boolean wifiInitialized = true;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    cycle -=1;
    if ( cycle == 0 ) {
      Serial.print("_end_");
      wifiInitialized = false;
      break;
    };
  }

  return wifiInitialized;
}
void startWifi() {
    WiFi.mode(WIFI_STA); // Aby nevysielal AP
    WiFi.begin(ssid, password);

}
void logTemp(float temperature, float humidity){
   
  // Zaloguj teplotu
    // Preved FLOAT na String
  char tmp[10];
  String teplota_namerana_string;
  String vlhkost_namerana_string;
  String urluri;
  
  teplota_namerana_string = String(temperature);
  vlhkost_namerana_string = String(humidity);

    // A mozes zalogovat
//  Serial.println(urluri);
  String logovanie = HttpGet("services.homeinfo.sk",80, "/a/loguj_ESP.php?priestor=" + miestnost + "&teplota=" + teplota_namerana_string + "&vlhkost=" + vlhkost_namerana_string + "&secret=polo_PZlPLnzlp59kFwf7");

    
//  Serial.println("Zalogoval som");
  
  // Cakaj
  //Serial.println("Spim 5 minut");
  //delay(300000);
  //delay(10000);
  
}


// MOJE FUNKCIE
String HttpGet (char* host, int httpPort, String url) {
  WiFiClient client;
  if (!client.connect(host, httpPort)) {
    Serial.println(">>> Wifi sa rozpojila!");
    return "0";
  }
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 15000) {
      Serial.println(">>> Timeout!");
      client.stop();
      return "0";
    }
  }
  while(client.available()){
    riadok = client.readStringUntil('\r');
//    Serial.println(line);
  }
  riadok.trim();
  String vystup = riadok;
  //Serial.println(vystup);
  return(vystup);
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("DHT ESP32 example with tasks");
  initTemp();
  initWiFi();
  // Signal end of setup() to tasks
  tasksEnabled = true;
}

void loop() {
  if (!tasksEnabled) {
    // Wait 2 seconds to let system settle down
    delay(2000);
    // Enable task that will read values from the DHT sensor
    tasksEnabled = true;
    if (tempTaskHandle != NULL) {
      initWiFi();
      vTaskResume(tempTaskHandle);
    }
  }
  yield();
}
