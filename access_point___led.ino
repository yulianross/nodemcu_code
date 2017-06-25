/*
 * WebSocketServer_LEDcontrol.ino
 *
 *  Created on: 26.11.2015
 *
 */
//Modified to act as AP
//arduino-er.blogspot.com

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>
#include <LinkedList.h>
#include <string>
#include <SocketIoClient.h>
#include "FS.h"

#define USE_SERIAL Serial


ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server = ESP8266WebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);
SocketIoClient webSocketIO;

String html_index;

void prepareFile(){
  
  Serial.println("Prepare file system");
  SPIFFS.begin();
  
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    Serial.println("file open failed");  
  } else{
    Serial.println("file open success");

    html_index = "";
    while (file.available()) {
      //Serial.write(file.read());
      String line = file.readStringUntil('\n');
      html_index += line + "\n";
    }
    file.close();
  }
}

LinkedList<String> splitMsg(String cadena, String key) {
  int size = 0;
  int index = 0;
  LinkedList<String> v;
  
  do {
    size = cadena.length();
    index = cadena.indexOf(key);
    v.add(cadena.substring(0,index));
    cadena = cadena.substring(index + 1, cadena.length());
  } while(index != -1);
  
  return v;
}

void messageEvent(const char * payload, size_t length) {
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(5000);                       // wait for 5 second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
}

void connectEvent(const char * payload, size_t length) {
   
   for (int i = 0; i < 4; i ++) {
    USE_SERIAL.printf("cliente conectado: %s\n", payload);
    digitalWrite(13, HIGH);
    delay(3000);
    digitalWrite(13, LOW);
    delay(1000);
   }  
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

            // send message to client
            //webSocket.sendTXT(num, "Connected");
        }
            break;
        case WStype_TEXT:
            if(payload[0] == '#') { 
              IPAddress ip = webSocket.remoteIP(num);
              webSocket.sendTXT(num, "connecting notifier to Wifi....if the connect process is succeeded the notifier will blink 4 times. In case the connection is failed, push reset button and try to connect again");
              WiFi.mode(WIFI_OFF);
                // we get data
                String msg = (char*)payload;
                String msgFiltered = msg.substring(1, msg.length());
        
                LinkedList<String> inputValues = splitMsg (msgFiltered, "/");
                WiFiMulti.addAP(string2char(inputValues.get(0)), string2char(inputValues.get(1)));

                while(WiFiMulti.run() != WL_CONNECTED) {
                  Serial.println("...");
                  delay(1000);
                }
             USE_SERIAL.println(string2char(inputValues.get(0)));
             webSocketIO.on(string2char(inputValues.get(2)), messageEvent);
             webSocketIO.on("connect", connectEvent);
             webSocketIO.begin("urlwebhook.herokuapp.com");
            }

            break;
    }
}

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

void setup() {
    USE_SERIAL.begin(115200);

    //USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    /*
    WiFiMulti.addAP("SSID", "passpasspass");

    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
    }
    */

    prepareFile();
    
    WiFi.softAP("arduino-er", "12345678");
    IPAddress myIP = WiFi.softAPIP();
    USE_SERIAL.print("AP IP address: ");
    USE_SERIAL.println(myIP);

    // start webSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    if(MDNS.begin("esp8266")) {
        USE_SERIAL.println("MDNS responder started");
    }

    // handle index
    server.on("/", []() {
        // send index.html
        // replace for better looking
        // es necesario añadir al principio '#' cuando envias los datos de los input, 
        server.send(200, "text/html", html_index);
    });
 
    server.begin();

    // Add service to MDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
    // initialize digital pin LED_BUILTIN as an output.
    pinMode(13, OUTPUT);
}

void loop() {
    webSocket.loop();
    webSocketIO.loop();
    server.handleClient();
}
