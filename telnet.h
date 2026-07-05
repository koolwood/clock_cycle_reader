/* ------------------------------------------------- */

//#include <Arduino.h>
#include "ESPTelnet.h"
 extern unsigned char telnetrx;
// /* ------------------------------------------------- */

// #define SERIAL_SPEED    9600
// #define WIFI_SSID       "MY SSID"
// #define WIFI_PASSWORD   "MY PASS"

// /* ------------------------------------------------- */


ESPTelnet telnet;
IPAddress ip;
uint16_t  port = 23;

/* ------------------------------------------------- */

// void setupSerial(long speed, String msg = "") {
//   Serial.begin(speed);
//   while (!Serial) {
//   }
//   delay(200);  
//   Serial.println();
//   Serial.println();
//   if (msg != "") Serial.println(msg);
// }

// /* ------------------------------------------------- */

// bool isConnected() {
//   return (WiFi.status() == WL_CONNECTED);
// }

/* ------------------------------------------------- */

// bool connectToWiFi(const char* ssid, const char* password, int max_tries = 20, int pause = 500) {
//   int i = 0;
//   WiFi.mode(WIFI_STA);
//   WiFi.disconnect();
//   delay(100);
  
//   #if defined(ARDUINO_ARCH_ESP8266)
//     WiFi.forceSleepWake();
//     delay(200);
//   #endif
//   WiFi.begin(ssid, password);
//   do {
//     delay(pause);
//     Serial.print(".");
//     i++;
//   } while (!isConnected() && i < max_tries);
//   WiFi.setAutoReconnect(true);
//   WiFi.persistent(true);
//   return isConnected();
// }

// /* ------------------------------------------------- */

void errorMsg(String error, bool restart = true) {
  Serial.println(error);
  if (restart) {
    Serial.println("Rebooting now...");
    delay(2000);
    ESP.restart();
    delay(2000);
  }
}

/* ------------------------------------------------- */

/* ------------------------------------------------- */

// (optional) callback functions for telnet events
void onTelnetConnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" connected");
  
  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("1=read,2=read next,3=read prev,4=toggle debug \n\r5=adjust treg,6=reset cycle,7=help\n\rbye=quit");
}

void onTelnetDisconnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" disconnected");
}

void onTelnetReconnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" reconnected");
}

void onTelnetConnectionAttempt(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" tried to connected");
}

void onTelnetInput(String str) {
  // checks for a certain command
  telnetrx = 0;
  if (str == "ping") {
    telnet.println("> pong"); 
    Serial.println("- Telnet: pong");
  // disconnect the client
  } else if (str == "bye") {
    telnet.println("> disconnecting you...");
    telnet.disconnectClient();
  } else if (str == "1") {
    telnet.println("> read cycle info");
    telnetrx = 'e';
  } else if (str == "2") {
    telnet.println("> next read cycle info");
    telnetrx = 'C';
  } else if (str == "3") {
    telnet.println(">previous read cycle info");
    telnetrx = 'c';
  } else if (str == "4") {
    telnet.println(">toggle debug");
    telnetrx = '1';
  } else if (str == "5") {
    telnet.println(">adjust trigger");
    telnetrx = 't';
  } else if (str == "s") {
    telnet.println(">get cnt");
    telnetrx = 's';
  } else if (str == "6") {
    telnet.println(">reset last cycle to 0");
    telnetrx = 'R';
  } else if (str == "8") {
    telnet.println(">print all cycles");
    telnetrx = 'L';
  } else if (str == "7") {
    telnet.println(">help");
    telnetrx = 'h';
  } else if (str == "reboot") {
    telnet.println(">rebooting in 2 seconds");
    delay(2000);
    ESP.restart();
    delay(2000);
  } else {
    telnet.println(str);
  }
}

/* ------------------------------------------------- */

// void setup() {
//   setupSerial(SERIAL_SPEED, "Telnet Test");
  
//   Serial.print("- Wifi: ");
//   connectToWiFi(WIFI_SSID, WIFI_PASSWORD);
  
//   if (isConnected()) {
//     ip = WiFi.localIP();
//     Serial.println();
//     Serial.print("- Telnet: "); Serial.print(ip); Serial.print(":"); Serial.println(port);
//     setupTelnet();
//   } else {
//     Serial.println();    
//     errorMsg("Error connecting to WiFi");
//   }
// }

/* ------------------------------------------------- */

void setupTelnet() {  
  // passing on functions for various telnet events
  telnet.onConnect(onTelnetConnect);
  telnet.onConnectionAttempt(onTelnetConnectionAttempt);
  telnet.onReconnect(onTelnetReconnect);
  telnet.onDisconnect(onTelnetDisconnect);
  telnet.onInputReceived(onTelnetInput);

  Serial.print("- Telnet: ");
  if (telnet.begin(port)) {
    Serial.println("running");
  } else {
    Serial.println("error.");
    errorMsg("Will reboot...");
  }
}

void teloop() {
  telnet.loop();

  // send serial input to telnet as output
  if (Serial.available()) {
    telnet.print(Serial.read());
  }
}

//* ------------------------------------------------- */