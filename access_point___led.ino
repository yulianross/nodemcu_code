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


#define USE_SERIAL Serial


ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server = ESP8266WebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);
SocketIoClient webSocketIO;

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
 
}

void connectEvent(const char * payload, size_t length) {
   USE_SERIAL.printf("cliente conectado: %s\n", payload);
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
            webSocket.sendTXT(num, "Connected");
        }
            break;
        case WStype_TEXT:
            if(payload[0] == '#') {
                // we get data
                String msg = (char*)payload;
                LinkedList<String> inputValues = splitMsg (msg, "/");

                //WiFiMulti.addAP(inputValues.get(0), inputValues.get(1));

                while(WiFiMulti.run() != WL_CONNECTED) {
                  delay(100);
                }

             char* event = string2char(inputValues.get(0));
             webSocketIO.on(event, messageEvent);
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
    //USE_SERIAL.begin(921600);
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
        server.send(200, "text/html", 
        "<html>"
        "<head>"
        "<script>"
        "var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);"
        "connection.onopen = function () {  connection.send('Connect ' + new Date()); };"
        "connection.onerror = function (error) {    console.log('WebSocket Error ', error);};"
        "connection.onmessage = function (e) {  console.log('Server: ', e.data);};"
        "function sendData() {  "
          "var wifi = document.getElementById('wifi').value;  "
          "var password = document.getElementById('pw').value;  "
          "var identifier = document.getElementById('identifier').value;  "
          "var data = '#'+wifi+'/'+password+'/'+identifier;    "
          "console.log('data: ' + data); "
          "connection.send(data);"
        "}"
        "</script>"
        "</head>"
        "<body>"
        "Connect identifier to wifi:<br/><br/>"
        "wifi: ""<input id=\"wifi\" type=\"text\" /><br/>"
        "password: <input id=\"pw\" type=\"text\" /><br/>"
        "identifier: <input id=\"identifier\" type=\"text\" /><br/>"
        "<button type=\"button\" onclick=\"sendData()\">connect</button>"
        "</body>"
      
        "</html>");
    });

    server.begin();

    // Add service to MDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
}

void loop() {
    webSocket.loop();
    webSocketIO.loop();
    server.handleClient();
}
