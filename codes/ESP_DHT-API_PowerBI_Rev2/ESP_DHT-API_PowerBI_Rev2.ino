/*
  by Yaser Ali Husen

  This code for sending data temperature and humidity to datastream PowerBI
  Step:
  1. In PowerBI workspace, create New Streaming semantic model, and copy the Push URL
  2. Change the PowerBI Push URL below

*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

//to get time NTP-----------------------------------
// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
const int timeZone = 7;     // WIB

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

String datetime;
//to get time NTP-----------------------------------

//Setup for DHT======================================
#include <DHT.h>
#define DHTPIN 2  //GPIO2 atau D4
// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT11     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;
//Setup for DHT======================================

const char* ssid = "WIFI SSID";
const char* password = "Password";

//millis================================
//Set every 5 sec read DHT
unsigned long previousMillis = 0;  // variable to store the last time the task was run
const long interval = 5000;        // time interval in milliseconds (eg 1000ms = 1 second)
//======================================

WiFiClient client;

void setup() {
  Serial.begin(9600);
  //For DHT sensor
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");

  //Start NTPClient
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
}

void loop() {
  delay(1000);
  unsigned long currentMillis = millis();  // mendapatkan waktu sekarang
  // Checks whether it is time to run the task
  if (currentMillis - previousMillis >= interval) {
    // Save the last time the task was run
    previousMillis = currentMillis;
    if (WiFi.status() == WL_CONNECTED) {
      //read DHT-11---------------------------------------
      t = dht.readTemperature();
      h = dht.readHumidity();
      Serial.print("Humidity = ");
      Serial.print(h);
      Serial.print("% ");
      Serial.print("Temperature = ");
      Serial.print(t);
      Serial.println(" C ");
      //read DHT-11---------------------------------------
      //get datetime
      getdatetime();
      
      String jsonString = "[{\"temperature\": " + String(t) + "\, \"humidity\": " + String(h) + ",\"datetime\": \"" + datetime +  "\"}]";
      
      HTTPClient http;
      WiFiClientSecure client;
      client.setInsecure();
      Serial.println(jsonString);
      
      if (!http.begin(client, "https://api.powerbi.com/beta/39758e22-eae4-4eeb-93fb-7e027070a602/datasets/51bff063-69f9-44c0-8d06-e6ec7f355aa1/rows?experience=power-bi&filter=Dashboards%2FDashboardGuid%20eq%20'60261bc0-6d37-4079-880e-02676723b4e5'&key=9ina%2FX2z72EikxZsohpDRzFTK50lHK18%2BqGFaDS6j2RYXPPI7FiYoxBwCrKm9C98LftEa59%2BS0oOMqXG7xByxg%3D%3D")) {
        Serial.println("BEGIN FAILED...");
      }

      http.addHeader("Content-Type", "application/json");
      int code = http.POST(jsonString);
      if (code < 0) {
        Serial.print("ERROR: ");
        Serial.println(code);
      }
      else {
        // Read response
        http.writeToStream(&Serial);
        Serial.flush();
      }

      // Disconnect
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
  }
}

void getdatetime()
{
  // display of the datetime
  datetime = String(year()) + "-" + String(month()) + "-" + String(day()) + " " + String(hour()) + ":" + String(minute()) + ":" + String(second());
  Serial.println(datetime);
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
