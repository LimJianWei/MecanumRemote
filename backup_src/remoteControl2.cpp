#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "C:\Users\Lim Jian Wei\Documents\PlatformIO\Projects\MecanumRemote\.pio\libdeps\nodemcuv2\WebSockets\src\WebSocketsServer.h"
#include <FS.h>

const byte DNS_PORT = 53;

IPAddress ip(192,168,1,1);
IPAddress subnet(192,168,1,1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);
WebSocketsServer webSocket(81);

//please dont do what did
//global variable is a mistake
bool g_input_flag = false;
float g_distance = 0;
float g_degree = 0;
float g_output_distance = 0;
float g_output_degree = 0;


void startWiFi();
void startDNS();
void startSPIFFS();
void startWebSocket();
void startServer();
String formatBytes(size_t bytes);
void handleNotFound();
bool handleFileRead(String path);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t lenght);
String getContentType(String filename);
void handleVector();

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
      break;
    case WStype_CONNECTED:
      //IPAddress foo = webSocket.remoteIP(num);
      break;
    case WStype_TEXT:
      //Serial.println((char*)payload);
      char* endPtr;
      //Serial.println(payload[0]);
      if(payload[0]=='L'||payload[0]=='R')//not important which joystick for now
      {
        float distance = strtod((const char*)&payload[1], &endPtr);
        float degree = strtod((const char*)endPtr, NULL);
        g_input_flag = true;
        g_distance = distance;
        g_degree = degree;
      }
      break;
  }
}

String getContentType(String filename) 
{ // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void handleVector()
{
  if(g_input_flag)
  {
    float A = g_output_distance*cosf(g_output_degree*PI/180) + g_distance*cosf(g_degree*PI/180);
    float B = g_output_distance*sinf(g_output_degree*PI/180) + g_degree*sinf(g_degree*PI/180);
    g_output_distance = sqrtf(A*A+B*B);
    g_output_degree = atanf(B/A);
    g_input_flag=false;
    Serial.print("out:\t");
    Serial.print(A);
    Serial.print("\t");
    Serial.print(B);
    Serial.print("\t");
    Serial.print(g_output_distance);
    Serial.print("\t");
    Serial.print(g_output_degree);
    Serial.print("\t");
    Serial.print(g_distance);
    Serial.print("\t");
    Serial.println(g_degree);
  }
  float dragStep = 7.0;
  float maxLimit = 1000.0;
  if (g_output_distance>0)
    if((g_output_distance-dragStep)>0)  g_output_distance-=dragStep;
    else  g_output_distance=0;
  else if (g_output_distance<0)
    if((g_output_distance+dragStep)<0)  g_output_distance+=dragStep;
    else  g_output_distance=0;
  if(g_output_distance>maxLimit)  g_output_distance=maxLimit;
  else if(g_output_distance<-maxLimit)  g_output_distance=-maxLimit;
}

void setup()
{
  Serial.begin(115200);
  pinMode(15, OUTPUT);
  startWiFi();
  startDNS();
  startSPIFFS();
  startWebSocket();
  startServer();
}

void loop()
{
  dnsServer.processNextRequest();
  webSocket.loop();
  webServer.handleClient();
  handleVector();
}
