#include <ESP8266WebServer.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <ESP8266Ping.h>
#include <PubSubClient.h>

#define SEALEVELPRESSURE_HPA (1013.25)
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

//const char* clientname = "lounge";
const char* clientname = "lounge";

//#define BMP_SCK 13
//#define BMP_MISO 12
//#define BMP_MOSI 11
//#define BMP_CS 10

Adafruit_BMP280 bme;

int restartercount = 0;
float temperaturebme, humidity, pressure, altitude, temperature, temperaturebme2, humidity2;

int pingcount;
long previousMillis = 0;
long interval = 10000;
int a = 60;

long systemUpTimeMn;
long systemUpTimeHr;
long systemUpTimeDy;
/*Put your SSID & Password*/
const char* ssid = "LittleFish";  // Enter SSID here
const char* password = "";  //Enter Password here


WiFiClient lounge;
PubSubClient client(lounge);
const char* mqtt_server = "192.168.0.178";
ESP8266WebServer server(80);
int startcount = 0;

// DHT Sensor
uint8_t DHTPin = D7;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

#define FIVEMIN (1000UL * 60 * 5);
unsigned long rolltime = millis() + FIVEMIN;


void setup() {
  Serial.begin(115200);
  delay(100);
  pinMode(DHTPin, INPUT);
  bme.begin(0x76);
  dht.begin();

  Serial.println("Connecting to ");
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

//  setup_wifi();
  client.setServer(mqtt_server, 1883);
//  client.setCallback(callback);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    if (startcount > 100) {
      Serial.print("stratcount-restart");
      ESP.restart();
    }
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  delay(10000);
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());
  //client.connect("lounge");
  delay(100);
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");

}

void pingChecker() {
  if(Ping.ping(WiFi.gatewayIP(), 1)) { // 1: Just one ping
  pingcount = 0;
  //Serial.println(Ping.averageTime()); // Unit: ms
  //Serial.println(" ms");
} else {
  pingcount++;
  Serial.println(" 1x ping count");
  if (pingcount > 6){
    delay(1000);
  ESP.restart();
  }
}
}

void sendpubsub(){
 if((long)(millis() - rolltime) >= 0) {
      char tempString[8];
      temperaturebme2 = bme.readTemperature();
      pressure = bme.readPressure() / 100.0F;
      dtostrf(temperaturebme2, 4, 2, tempString);
      char presString[8];
      dtostrf(pressure, 2, 2, presString);
      humidity2 = dht.readHumidity();
      pressure = bme.readPressure() / 100.0F;
      char humidString[8];
      dtostrf(humidity2, 4, 2, humidString);
      client.connect(clientname);
      delay(100);
          client.publish("lounge/temperature", tempString);
          Serial.println(tempString);
          client.publish("lounge/pressure", presString);
          Serial.println(presString);
          client.publish("lounge/humidity", humidString);
          Serial.println(humidString);
          Serial.println("pubsubsent");
      rolltime += FIVEMIN;
      Serial.println("rolltime rolls over");
    }
}

void publishSystemUptime() {
  long millisecs = millis();
  systemUpTimeMn = int((millisecs / (1000 * 60)) % 60);
  systemUpTimeHr = int((millisecs / (1000 * 60 * 60)) % 24);
  systemUpTimeDy = int((millisecs / (1000 * 60 * 60 * 24)) % 365);
}

void loop() {
  server.handleClient();
  delay(10);
  restartercount++;
  publishSystemUptime();
  sendpubsub();

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis > interval) {
      previousMillis = currentMillis;
      //Serial.println("Test");
      //Serial.println(previousMillis);
      //Serial.println(a);
      pingChecker();
      }

  if (restartercount > 1600000 || (WiFi.status() != WL_CONNECTED)) {
    Serial.print("wifi disconect");
    delay(1000);
    ESP.restart();
  }
}

void handle_OnConnect() {
  temperaturebme = bme.readTemperature();
  //humidity = bme.readHumidity();
  humidity = dht.readHumidity();
  pressure = bme.readPressure() / 100.0F;
  altitude = dht.readTemperature();
  temperature = (altitude - 1);
  server.send(200, "text/html", SendHTML(temperature, humidity, pressure, altitude));
  //delay(3000);
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

String SendHTML(float temperature, float humidity, float pressure, float altitude) {
  String ptr = "<!DOCTYPE html>";
  ptr += "<html>";
  ptr += "<head>";
  ptr += "<title>lounge Weather Station</title>";
  ptr += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  ptr += "<style>";
  ptr += "html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #444444;}";
  ptr += "body{margin: 0px;} ";
  ptr += "h1 {margin: 50px auto 30px;} ";
  ptr += ".side-by-side{display: table-cell;vertical-align: middle;position: relative;}";
  ptr += ".text{font-weight: 600;font-size: 19px;width: 200px;}";
  ptr += ".reading{font-weight: 300;font-size: 50px;padding-right: 25px;}";
  ptr += ".temperature .reading{color: #F29C1F;}";
  ptr += ".humidity .reading{color: #3B97D3;}";
  ptr += ".pressure .reading{color: #26B99A;}";
  ptr += ".altitude .reading{color: #955BA5;}";
  ptr += ".superscript{font-size: 17px;font-weight: 600;position: absolute;top: 10px;}";
  ptr += ".data{padding: 10px;}";
  ptr += ".container{display: table;margin: 0 auto;}";
  ptr += ".icon{width:65px}";
  ptr += "</style>";
  ptr += "<script>\n";
  ptr += "setInterval(loadDoc,30000);\n";
  ptr += "function loadDoc() {\n";
  ptr += "var xhttp = new XMLHttpRequest();\n";
  ptr += "xhttp.onreadystatechange = function() {\n";
  ptr += "if (this.readyState == 4 && this.status == 200) {\n";
  ptr += "document.body.innerHTML =this.responseText}\n";
  ptr += "};\n";
  ptr += "xhttp.open(\"GET\", \"/\", true);\n";
  ptr += "xhttp.send();\n";
  ptr += "}\n";
  ptr += "</script>\n";
  ptr += "</head>";
  ptr += "<body>";
  ptr += "<h1>lounge weather station</h1>";
  ptr += "<div class='container'>";
  ptr += "<div class='data pressure'>";
  ptr += "<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 40.542 40.541'height=40.541px id=Layer_1 version=1.1 viewBox='0 0 40.542 40.541'width=40.542px x=0px xml:space=preserve y=0px><g><path d='M34.313,20.271c0-0.552,0.447-1,1-1h5.178c-0.236-4.841-2.163-9.228-5.214-12.593l-3.425,3.424";
  ptr += "c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293c-0.391-0.391-0.391-1.023,0-1.414l3.425-3.424";
  ptr += "c-3.375-3.059-7.776-4.987-12.634-5.215c0.015,0.067,0.041,0.13,0.041,0.202v4.687c0,0.552-0.447,1-1,1s-1-0.448-1-1V0.25";
  ptr += "c0-0.071,0.026-0.134,0.041-0.202C14.39,0.279,9.936,2.256,6.544,5.385l3.576,3.577c0.391,0.391,0.391,1.024,0,1.414";
  ptr += "c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293L5.142,6.812c-2.98,3.348-4.858,7.682-5.092,12.459h4.804";
  ptr += "c0.552,0,1,0.448,1,1s-0.448,1-1,1H0.05c0.525,10.728,9.362,19.271,20.22,19.271c10.857,0,19.696-8.543,20.22-19.271h-5.178";
  ptr += "C34.76,21.271,34.313,20.823,34.313,20.271z M23.084,22.037c-0.559,1.561-2.274,2.372-3.833,1.814";
  ptr += "c-1.561-0.557-2.373-2.272-1.815-3.833c0.372-1.041,1.263-1.737,2.277-1.928L25.2,7.202L22.497,19.05";
  ptr += "C23.196,19.843,23.464,20.973,23.084,22.037z'fill=#26B999 /></g></svg>";
  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Pressure</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (float)pressure;
  ptr += "<span class='superscript'>hPa</span></div>";
  ptr += "</div>";
  ptr += "<div class='data temperature'>";
  ptr += "<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve  y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
  ptr += "C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
  ptr += "c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
  ptr += "c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
  ptr += "s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>";
  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Temperature BME sensor</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (float)temperaturebme;
  ptr += "<span class='superscript'>&deg;C</span></div>";
  ptr += "</div>";
  /*
    ptr +="<div class='data uptime'>";
    ptr +="<div class='side-by-side icon'>";
    ptr +="<svg enable-background='new 0 0 58.422 40.639'height=40.639px id=Layer_1 version=1.1 viewBox='0 0 58.422 40.639'width=58.422px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M58.203,37.754l0.007-0.004L42.09,9.935l-0.001,0.001c-0.356-0.543-0.969-0.902-1.667-0.902";
    ptr +="c-0.655,0-1.231,0.32-1.595,0.808l-0.011-0.007l-0.039,0.067c-0.021,0.03-0.035,0.063-0.054,0.094L22.78,37.692l0.008,0.004";
    ptr +="c-0.149,0.28-0.242,0.594-0.242,0.934c0,1.102,0.894,1.995,1.994,1.995v0.015h31.888c1.101,0,1.994-0.893,1.994-1.994";
    ptr +="C58.422,38.323,58.339,38.024,58.203,37.754z'fill=#955BA5 /><path d='M19.704,38.674l-0.013-0.004l13.544-23.522L25.13,1.156l-0.002,0.001C24.671,0.459,23.885,0,22.985,0";
    ptr +="c-0.84,0-1.582,0.41-2.051,1.038l-0.016-0.01L20.87,1.114c-0.025,0.039-0.046,0.082-0.068,0.124L0.299,36.851l0.013,0.004";
    ptr +="C0.117,37.215,0,37.62,0,38.059c0,1.412,1.147,2.565,2.565,2.565v0.015h16.989c-0.091-0.256-0.149-0.526-0.149-0.813";
    ptr +="C19.405,39.407,19.518,39.019,19.704,38.674z'fill=#955BA5 /></g></svg>";
    ptr +="</div>";
    ptr +="<div class='side-by-side text'>System Uptime</div>";
    ptr +="<div class='side-by-side reading'>";
    ptr +=(int)systemUpTimeMn;
    ptr +="<span class='superscript'>mins</span></div>";
    ptr +="</div>";
  */
  ptr += "<div class='data uptimehr'>";
  ptr += "<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 58.422 40.639'height=20.639px id=Layer_1 version=1.1 viewBox='0 0 58.422 40.639'width=58.422px x=0px xml:space=preserve y=0px><g><path d='M58.203,37.754l0.007-0.004L42.09,9.935l-0.001,0.001c-0.356-0.543-0.969-0.902-1.667-0.902";
  ptr += "c-0.655,0-1.231,0.32-1.595,0.808l-0.011-0.007l-0.039,0.067c-0.021,0.03-0.035,0.063-0.054,0.094L22.78,37.692l0.008,0.004";
  ptr += "c-0.149,0.28-0.242,0.594-0.242,0.934c0,1.102,0.894,1.995,1.994,1.995v0.015h31.888c1.101,0,1.994-0.893,1.994-1.994";
  ptr += "C58.422,38.323,58.339,38.024,58.203,37.754z'fill=#955BA5 /><path d='M19.706,38.674l-0.013-0.004l13.544-23.522L25.13,1.156l-0.002,0.001C24.671,0.459,23.885,0,22.985,0";
  ptr += "c-0.84,0-1.582,0.41-2.051,1.038l-0.016-0.01L20.87,1.114c-0.025,0.039-0.046,0.082-0.068,0.124L0.299,36.851l0.013,0.004";
  ptr += "C0.117,37.215,0,37.62,0,38.059c0,1.412,1.147,2.565,2.565,2.565v0.015h16.989c-0.091-0.256-0.149-0.526-0.149-0.813";
  ptr += "C19.405,39.407,19.518,39.019,19.704,38.674z'fill=#955BA5 /></g></svg>";
  ptr += "</div>";
  ptr += "<div class='side-by-side text'>System Uptime</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)systemUpTimeDy;
  ptr += ",  ";
  ptr += (int)systemUpTimeHr;
  ptr += ":";
  ptr += (int)systemUpTimeMn;
  ptr += "<span class='superscript'>D, HH:MM</span></div>";
  ptr += "</div>";
  ptr += "</body>";
  ptr += "</html>";
  return ptr;
}
