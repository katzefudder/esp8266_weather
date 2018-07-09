#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <Adafruit_BMP085.h>
#include <ntp.h>
#include <TimeLib.h>
#include <ACROBOTIC_SSD1306.h>


#define WLAN_AP "accesspoint"
#define WLAN_AP_PW "password"

#define DHTPIN 5
#define DHTTYPE DHT22
#define GMT -1 //timezone

DHT_Unified dht(DHTPIN, DHTTYPE);

#define DEFAULT_PAUSE 10 // seconds to pause
#define GRAPHITE_IP 127,0,0,1
#define GRAPHITE_PORT 2003

ESP8266WiFiMulti WiFiMulti;
Adafruit_BMP085 bmp;

time_t getNTPtime(void);
NTP NTPclient;
WiFiUDP Udp;
IPAddress remoteIP(GRAPHITE_IP); //graphite server

unsigned int remotePort = GRAPHITE_PORT;  //graphite server port
unsigned int localPort = 2003;

void setup() {

  Serial.begin(9600);
  delay(200);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFiMulti.addAP(WLAN_AP, WLAN_AP_PW);

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while(WiFiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Udp.begin(localPort);
  
  while(timeStatus() == timeNotSet) {
    Serial.println("waiting for ntp");
    NTPclient.begin("europe.pool.ntp.org", GMT);
    setSyncInterval(SECS_PER_HOUR);
    setSyncProvider(getNTPtime);
    delay(1000);
  }
  
  //blinking led if wifi is connected and time is synced
  for (int i=1;i<10;i++){
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
  }

  Wire.begin(2, 3);
  oled.init();                      // Initialze SSD1306 OLED display
  //oled.setFont(font5x7);            // Set font type (default 8x8)
  oled.clearDisplay();              // Clear screen

  if(!bmp.begin()) {
    Serial.print("Ooops, no BMP180 detected ... Check your wiring or I2C ADDR!");
    while(1);
  } else {
    Serial.println("BMP180 detected");
  }
}

void loop() {
  sendLightHumidityTemperature();
  sendBmpData();
  
  // pause
  delay(DEFAULT_PAUSE * 1000);
}

void sendBmpData() {
  float pressure = bmp.readPressure();
  float altitude = bmp.readAltitude();
  float realAltitude = bmp.readAltitude(100200);
  float seaLevelPressure = bmp.readSealevelPressure();

  sendUdpPacket("systems.nodemcu_2.pressure ", pressure);
  sendUdpPacket("systems.nodemcu_2.altitude ", altitude);
  sendUdpPacket("systems.nodemcu_2.real_altitude ", realAltitude);
  sendUdpPacket("systems.nodemcu_2.sea_level_altitude ", seaLevelPressure);
}

void sendLightHumidityTemperature() {
  int timestamp = now();
  int light = analogRead(A0);

  Serial.println(light);

  dht.begin();
  
  sensor_t sensor;
  
  sensors_event_t event;
  
  dht.temperature().getEvent(&event);
  float temperature = event.temperature;

  if (isnan(temperature)) {
    Serial.println("Error reading temperature!");
  }

  dht.humidity().getEvent(&event);
  float humidity = event.relative_humidity;

  

  if (isnan(humidity)) {
    Serial.println("Error reading humidity!");
  }

  sendUdpPacket("systems.nodemcu_2.light ", light);
  sendUdpPacket("systems.nodemcu_2.temperature ", temperature);
  sendUdpPacket("systems.nodemcu_2.humidity ", humidity);

  char graphite_rssi[50];
  int rssi = WiFi.RSSI();
  String ssid = WiFi.SSID();
  String graphite_data = "systems.nodemcu_2.rssi." + ssid + " ";
  graphite_data.toCharArray(graphite_rssi, 50);
  Udp.beginPacket(remoteIP, remotePort);
  Udp.write(graphite_rssi);
  Udp.print(rssi);
  Udp.write(" ");
  Udp.println(timestamp);
  Udp.endPacket();

  oled.setTextXY(0,0);
  oled.putString("T: ");
  oled.setTextXY(0,3);
  oled.putFloat(temperature);
  oled.setTextXY(0,8);
  oled.putString("C");
  
  oled.setTextXY(1,0);
  oled.putString("H: ");
  oled.setTextXY(1,3);
  oled.putFloat(humidity, 2);
  oled.setTextXY(1,8);
  oled.putString("%");
  
  oled.setTextXY(2,0);
  oled.putString("L: ");
  oled.setTextXY(2,3);
  oled.putFloat(light, 2);

  oled.setTextXY(3,0);
  oled.putString("W: ");
  oled.setTextXY(3,3);
  oled.putFloat(rssi);

  Serial.println("data sent");
}

void sendUdpPacket(char* key, float value) {
  int timestamp = now();
  Udp.beginPacket(remoteIP, remotePort);
  Udp.write(key);
  Udp.print(value);
  Udp.write(" ");
  Udp.println(timestamp);
  Udp.endPacket();
}

time_t getNTPtime(void) {
  return NTPclient.getNtpTime();
}
