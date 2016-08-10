// MotionCam: motion-activated ESP8266 + Arducam camera thing, with self-hosted and push-to-cloud mode.
//
// Notes:
// 1 - This thing doesn't speak HTTP when doing motion cap posts - it's much simpler. See README.md
//
// Crudely stolen from:
// ArduCAM Mini demo (C)2016 Lee web: http://www.ArduCAM.com
// This program is a demo of how to use most of the functions of the library with ArduCAM ESP8266 2MP camera.

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "DNSServer.h"
#include "memorysaver.h"

//
// CONFIG HERE:
//
const int camResolution = OV2640_320x240;
const int camCSPin = D3;                  // this pin is the slave selection pin for the camera 
int wifiType = 0; // 0:Station  1:AP
const char* ssid = "camera"; // access point name - will be joined (station) or created (AP)
const char* password = ""; // password for access point
IPAddress apIPAddr(192,168,42,1); // when acting as access point, what IP should this device have?
const int useMotionSensor = 1;
const int motionSensorPin = D0;
const int motionCapDelay = 100;
const char* postHost = "txpo.st"; // where to send motion sensor photo bursts - see note 1 above!
const int postPort = 7654;
const int photoBurstCount=3;
const int photoBurstSleep=100;
//
// END CONFIG
//

WiFiClient postConn;
#define DNS_PORT 53
DNSServer dnsServer;
ESP8266WebServer server(80);
ArduCAM camObj(OV2640, camCSPin);
byte lastMoReading = LOW;

void camInit() {
  uint8_t vid, pid;
  uint8_t temp;
#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif
  Serial.begin(115200);
  Serial.println("ArduCAM Start!");
  // set the CS as an output:
  pinMode(camCSPin, OUTPUT);
  // initialize SPI:
  SPI.begin();
  SPI.setFrequency(8000000); //8MHz
  //Check if the ArduCAM SPI bus is OK
  camObj.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = camObj.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55){
    Serial.println("SPI1 interface Error!");
    while(1);
  }
  //Check if the camera module type is OV2640
  camObj.wrSensorReg8_8(0xff, 0x01);
  camObj.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  camObj.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
    Serial.println("Can't find OV2640 module!");
  else
    Serial.println("OV2640 detected.");
  //Change to JPEG capture mode and initialize the OV2640 module
  camObj.set_format(JPEG);
  camObj.InitCAM();
  camObj.OV2640_set_JPEG_size(camResolution);
  camObj.clear_fifo_flag();
  camObj.write_reg(ARDUCHIP_FRAMES, 0x00);
}

size_t camCapFrame(){
  size_t t1 = millis();
  camObj.clear_fifo_flag();
  camObj.start_capture();
  while (!camObj.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  size_t len = camObj.read_fifo_length();
  if (len >= 393216){ Serial.println("Over size."); return 0;}
  else if (len == 0){ Serial.println("Size is 0."); return 0;}
  Serial.println("pic size = " + String(len) + " bytes; time = " + String(millis() - t1) + " ms"); 
  camObj.CS_LOW(); camObj.set_fifo_burst(); SPI.transfer(0xFF);
  return len;
}

void serverCapture(){
  WiFiClient client = server.client();
  size_t len = camCapFrame();
  if (!len) return;
  if (!client.connected()) return;
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: image/jpeg\r\n";
  response += "Content-Length: " + String(len) + "\r\n\r\n";
  server.sendContent(response);
  static const size_t bufferSize = 2048;
  static uint8_t buffer[bufferSize] = {0xFF};
  while (len) {
      size_t will_copy = (len < bufferSize) ? len : bufferSize;
      SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
      if (!client.connected()) break;
      client.write(&buffer[0], will_copy);
      len -= will_copy;
  }
  camObj.CS_HIGH();
}

void serverStream(){
  WiFiClient client = server.client();
  
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);
  
  while (1){
    size_t len = camCapFrame();
    if (!len) break;
    if (!client.connected()) break;
    response = "--frame\r\n";
    response += "Content-Type: image/jpeg\r\n\r\n";
    server.sendContent(response);
    static const size_t bufferSize = 2048;
    static uint8_t buffer[bufferSize] = {0xFF};
    while (len) {
      size_t will_copy = (len < bufferSize) ? len : bufferSize;
      SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
      if (!postConn.connected()) break;
      postConn.write(&buffer[0], will_copy);
      len -= will_copy;
    }
    camObj.CS_HIGH();
    if (!client.connected()) break;
  }
}

void pushPhoto(){
  delay(motionCapDelay);
  size_t len = camCapFrame();
  if (!len) return;
  if (!postConn.connect(postHost, postPort)) {
    Serial.println("ERR: could not connect to "+String(postHost)+" "+String(postPort));
    return;
  }
  while(postConn.available()) {String line = postConn.readStringUntil('\r');}  // Empty wifi receive bufffer    
  uint16_t full_length;
  static const size_t bufferSize = 1024; // original value 4096 caused split pictures
  static uint8_t buffer[bufferSize] = {0xFF};
  while (len) {
      size_t will_copy = (len < bufferSize) ? len : bufferSize;
      SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
      if (!postConn.connected()) { Serial.println("HTTP POST connection broken"); break; }
      postConn.write(&buffer[0], will_copy);
      Serial.println(".. streamed "+String(will_copy));
      len -= will_copy;
  }
  camObj.CS_HIGH();
  postConn.stop();  
}
void handleNotFound(){
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text/plain", message);
  if (server.hasArg("ql")){
    int ql = server.arg("ql").toInt();
    camObj.OV2640_set_JPEG_size(ql);
    Serial.println("QL change to: " + server.arg("ql"));
  }
}
void setupNet() {
  Serial.println();
  Serial.println("Network..");
  if (wifiType == 0){
    // Connect to WiFi network
    Serial.println("connecting: " + String(ssid));
    WiFi.mode(WIFI_AP);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("connected. ip = " + WiFi.localIP());
  } else if (wifiType == 1) {
    Serial.println("creating access point: " + String(ssid));
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIPAddr, apIPAddr, IPAddress(255, 255, 255, 0));
    WiFi.softAP(ssid, password);
    dnsServer.start(DNS_PORT, "*", apIPAddr);
    Serial.println("online. ip = " + WiFi.softAPIP());
  }
}

void setupServer() {
  // Start the server
  server.on("/capture", HTTP_GET, serverCapture);
  server.on("/", HTTP_GET, serverStream);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Server started");  
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(motionSensorPin, INPUT);
  camInit();
  setupNet();
  setupServer();
}

void loop() {
  byte pin;
  if(useMotionSensor && (pin=digitalRead(motionSensorPin))!=lastMoReading) {
    digitalWrite(BUILTIN_LED,pin);
    lastMoReading=pin;
    if(pin==HIGH) {
      byte i;
      for (i=0; i<photoBurstCount; i++) {
        pushPhoto();
        delay(photoBurstSleep); // this is stupid; we should keep the server connection open
      }
    }
  }
  server.handleClient();
  dnsServer.processNextRequest();
}

