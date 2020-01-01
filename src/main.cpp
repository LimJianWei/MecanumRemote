#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "..\.pio\libdeps\nodemcuv2\WebSockets\src\WebSocketsServer.h"
#include <FS.h>
#include <cmath>

const byte DNS_PORT = 53;
const int pinPWM[]={5,4,0,15};//1,2,3,4
const int pinDirection[]={14,12,13,2};//5,6,7,8

//im sorry global variable
float prev_distance = 0.0f;
float prev_degree = 0.0f;
bool endInputPollFlag = false;

IPAddress ip(192,168,1,1);
IPAddress subnet(192,168,1,1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);
WebSocketsServer webSocket(81);

void startWiFi();
void startDNS();
void startSPIFFS();
void startWebSocket();
void startServer();
String formatBytes(size_t bytes);
void handleNotFound();
bool handleFileRead(String path);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t lenght);
void handleShiftDir(float distance, float degree, bool isitR);
float calcShiftDir(float degree);
String getContentType(String filename);

void startWiFi()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip, ip, IPAddress(255,255,255,0));
  WiFi.softAP("Omniwheel", "PoopShit");//Name, Password
}

void startDNS()
{
  dnsServer.setTTL(300);
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(DNS_PORT, "www.omniwheel.com", ip);
}

void startSPIFFS()
{
  SPIFFS.begin();
}

void startWebSocket()
{
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void startServer()
{
  webServer.onNotFound(handleNotFound);
  webServer.begin();
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

String formatBytes(size_t bytes) 
{ // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

void handleNotFound()
{
  if(!handleFileRead(webServer.uri()))
    webServer.send(404, "text/plain", "404: File Not Found");
}

bool handleFileRead(String path)
{
  bool flag = false;
  if(path.endsWith("/"))
    path+="index.html";
  String contentType = getContentType(path);
  if(SPIFFS.exists(path+".gz"))
    path+=".gz";
  if(SPIFFS.exists(path))
  {
    File file = SPIFFS.open(path, "r");
    size_t sent = webServer.streamFile(file, contentType);
    file.close();
    flag = true;
  }
  return flag;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t lenght)
{
  switch(type)
  {
    case WStype_DISCONNECTED:
      endInputPollFlag = true;
      break;
    case WStype_CONNECTED:
      //IPAddress foo = webSocket.remoteIP(num);
      break;
    case WStype_TEXT:
      Serial.println((char*)payload);
      char* endPtr;
      //Serial.println(payload[0]);
      if(payload[0]=='L'||payload[0]=='R')//not important which joystick for now
      {
        if(payload[1]=='E')
          endInputPollFlag = true;
        else
        {
          endInputPollFlag = false;
          float distance = strtod((const char*)&payload[1], &endPtr);
          float degree = strtod((const char*)endPtr, NULL);
          //replace previous data
          prev_distance = distance;
          prev_degree = degree;
          bool isitR = false;
          if(payload[0]=='R')
            isitR = true;
          handleShiftDir(distance, degree, isitR);
        }
      }
      break;
  }
}

//Test this with care, liability when robot car goes wild
//when finger is let go, linearly decrease speed to prevent casualty
void handleEndInputPoll()
{
  const float rateOfSlowing = 1.0f;//linear decay of 1.0f looks smooth
  if(endInputPollFlag)
  {
    ((prev_distance-rateOfSlowing)>0)?(prev_distance-=rateOfSlowing):(prev_distance=0);
    handleShiftDir(prev_distance, prev_degree, 0);
  }
}

//Anti-/Clockwise WIP
//computate each wheel going in shifting mode, and outputs it
//be warned that the logic doesnt make any sense
void handleShiftDir(float distance, float degree, bool isitR)
{
  const float pwmAmplitude = 255.0f;//1023
  if(!isitR)//Left
  {
    //distance and degree to each wheel to scale of -1<x<1
    float wheel14 = distance/100.0f*calcShiftDir(degree);
    float wheel23 = distance/100.0f*calcShiftDir(degree+270.0f);
    //get sign and absolute value for each wheel
    bool sign_wheel14=!(wheel14<0);
    bool sign_wheel23=!(wheel23<0);
    float abs_wheel14=fabsf(wheel14);;//big brain moment
    float abs_wheel23=fabsf(wheel23);
    digitalWrite(pinDirection[0], !sign_wheel14);
    digitalWrite(pinDirection[1], sign_wheel23);
    digitalWrite(pinDirection[2], !sign_wheel23);
    digitalWrite(pinDirection[3], sign_wheel14);
    analogWrite(pinPWM[0], static_cast<int>(pwmAmplitude*abs_wheel14));
    analogWrite(pinPWM[1], static_cast<int>(pwmAmplitude*abs_wheel23));
    analogWrite(pinPWM[2], static_cast<int>(pwmAmplitude*abs_wheel23));
    analogWrite(pinPWM[3], static_cast<int>(pwmAmplitude*abs_wheel14));
  }
  else//Right
  {
    
    float wheel13 = distance/100.0f*calcShiftDir(degree+180.0f);
    float wheel24 = distance/100.0f*calcShiftDir(degree+90.0f);
    bool sign_wheel13=!(wheel13<0);
    bool sign_wheel24=!(wheel24<0);
    float abs_wheel13=fabsf(wheel13);;//big brain moment
    float abs_wheel24=fabsf(wheel24);
    digitalWrite(pinDirection[0], sign_wheel13);
    digitalWrite(pinDirection[1], !sign_wheel24);
    digitalWrite(pinDirection[2], sign_wheel13);
    digitalWrite(pinDirection[3], !sign_wheel24);
    analogWrite(pinPWM[0], static_cast<int>(pwmAmplitude*abs_wheel13));
    analogWrite(pinPWM[1], static_cast<int>(pwmAmplitude*abs_wheel24));
    analogWrite(pinPWM[2], static_cast<int>(pwmAmplitude*abs_wheel13));
    analogWrite(pinPWM[3], static_cast<int>(pwmAmplitude*abs_wheel24));
  }
}

//computate using degree to direction of wheel
float calcShiftDir(float degree)
{
  float shiftDir = 1.0f;
  degree=fmodf(degree,360.0f);
  if(degree>=90.0f) shiftDir+=(degree-90.0f)/-45.0f;
  if(degree>=180.0f) shiftDir+=(degree-180.0f)/45.0f;
  if(degree>=270.0f) shiftDir+=(degree-270.0f)/45.0f;
  return shiftDir;
}

String getContentType(String filename) 
{ 
  // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void setup()
{
  for(auto x : pinDirection)
  {
    pinMode(x, OUTPUT);
    digitalWrite(x, LOW);
  }
  for(auto x : pinPWM)
  {
    pinMode(x, OUTPUT);
    digitalWrite(x, LOW);
  }
  Serial.begin(115200);
  startWiFi();
  startDNS();
  startSPIFFS();
  startWebSocket();
  startServer();
}

void loop()
{
  for(int i=0;i<75;i++)//to slow down handler for handleEndInputPoll
  {
    dnsServer.processNextRequest();
    webSocket.loop();
    webServer.handleClient();
  }
  handleEndInputPoll();
}
