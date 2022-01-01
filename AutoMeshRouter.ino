//************************************************************
// Source code for the Autonomous Wireless Mesh Router
//
// By: Benjamin Stevens (02078018)
//     Pattrick Matthew ()
//     Luke Alexander   ()
//     Kevan Yi Chhor   ()
//     Min Khoi         ()
//     Maria Herrera    ()
//
// Adapted from the "basic" example included with the painlessmesh Arduino
// libary and the "WiFi_LoRa_32FactoryTest" example included with the heltec
// Arduino library.
//
//************************************************************

#include "Arduino.h"
#include "heltec.h"
#include "images.h"
#include "painlessMesh.h"

// painlessmesh already utilises to WiFi.h
//#include "WiFi.h"

#define MESH_PREFIX    "AutoMeshWLAN"
#define MESH_PASSWORD  "AutoMeshWLAN"
#define MESH_PORT      5555
#define BAND           915E6 //LoRa frequency used in Australia is 915 megahertz
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

unsigned int counter = 0;
bool receiveflag = false; // software flag for LoRa receiver, received data makes it true.
long lastSendTime = 0;    // last send time
int interval = 1000;      // interval between sends
uint64_t chipid;

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

// For displaying the Heltec logo.
void logo(){
  Heltec.display -> clear();
  Heltec.display -> drawXbm(0,5,logo_width,logo_height,(const unsigned char *)logo_bits);
  Heltec.display -> display();
}

void sendMessage() {
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  mesh.sendBroadcast( msg );
  taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {

  // Display messages addressed to this node.

  String message = "Message: " + msg + " received from node ID" + String(from);
  Heltec.display -> drawString(0, 0, message);
  Heltec.display -> display();
  delay(2000);
  Heltec.display -> clear();
  //Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {

  String message = "New connection made from node ID " + String(nodeId);
  Heltec.display -> drawString(0, 0, message);
  Heltec.display -> display();
  delay(2000);
  Heltec.display -> clear();
  //Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Heltec.display -> drawString(0, 0, "Mesh connections have changed.");
  Heltec.display -> display();
  delay(2000);
  Heltec.display -> clear();
  //Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  String message = "Time adjusted to " + String(mesh.getNodeTime()) + " and offset changed to " + String(offset);
  Heltec.display -> drawString(0, 0, message);
  Heltec.display -> display();
  delay(2000);
  Heltec.display -> clear();
  //Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() {
  //Serial.begin(115200);

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
//mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  delay(6000000);
  esp_deep_sleep_start();
  
}

void loop() {
  // This will run the user scheduler as well.
  mesh.update();
}
