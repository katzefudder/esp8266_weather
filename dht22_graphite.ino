#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti WiFiMulti;
#include <WiFiUdp.h>

#define WLAN_AP "YOUR_WIFI"
#define WLAN_AP_PW "YOUR_WIFI_PASSWORD"

#include "DHT.h"
#define DHTPIN 5
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define DEFAULT_PAUSE 10 // seconds to pause
#define GRAPHITE_IP 52,57,84,177
#define GRAPHITE_PORT 2003

#include <ntp.h>
#include <TimeLib.h>
#define GMT -1 //timezone

time_t getNTPtime(void);
NTP NTPclient;
WiFiUDP Udp;
IPAddress remoteIP(GRAPHITE_IP); //graphite server

unsigned int remotePort = GRAPHITE_PORT;  //graphite server port
unsigned int localPort = 2003;

void setup() {

  Serial.begin(115200);
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


}

void loop() {
  collectAndSendData();
  // pause 15sec
  delay(DEFAULT_PAUSE * 1000);
}

void collectAndSendData() {
  int light = analogRead(A0);
  float humidity = dht.readHumidity();
  float temp = dht.readTemperature();

  int timestamp = now();

  Udp.beginPacket(remoteIP, remotePort);
  Udp.write("systems.nodemcu.light ");
  Udp.print(light);
  Udp.write(" ");
  Udp.println(timestamp);
  Udp.endPacket();

  Udp.beginPacket(remoteIP, remotePort);
  Udp.write("systems.nodemcu.temp ");
  Udp.print(temp);
  Udp.write(" ");
  Udp.println(timestamp);
  Udp.endPacket();

  Udp.beginPacket(remoteIP, remotePort);
  Udp.write("systems.nodemcu.humidity ");
  Udp.print(humidity);
  Udp.write(" ");
  Udp.println(timestamp);
  Udp.endPacket();

  char graphite_rssi[50];
  int rssi = WiFi.RSSI();
  String ssid = WiFi.SSID();
  String graphite_data = "systems.nodemcu.rssi." + ssid + " ";
  graphite_data.toCharArray(graphite_rssi, 50);
  Udp.beginPacket(remoteIP, remotePort);
  Udp.write(graphite_rssi);
  Udp.print(rssi);
  Udp.write(" ");
  Udp.println(timestamp);
  Udp.endPacket();

  // show signal that data has been sent
  digitalWrite(LED_BUILTIN, LOW);
  delay(50);
  digitalWrite(LED_BUILTIN, HIGH);
}

time_t getNTPtime(void)
{
  return NTPclient.getNtpTime();
}

