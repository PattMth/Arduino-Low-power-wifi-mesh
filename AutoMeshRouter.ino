/************************************************************
 * Source code for the Autonomous Wireless Mesh Router
 *
 * By: Benjamin Stevens         (02078018)
 *     Pattrick Matthew Hartono (13652452)
 *
 * Adapted from the "basic" example included with the painlessmesh Arduino
 * libary as well as the "WiFi_LoRa_32FactoryTest" and "Low_Power" examples
 * included with the heltec Arduino library.
 *
 * Painlessmesh basic source code.
 * https://github.com/gmag11/painlessMesh/blob/master/examples/basic/basic.ino - 
 *
 * WiFi_LoRa_32FactoryTest source code.
 * https://github.com/HelTecAutomation/Heltec_ESP32/blob/master/examples/Factory_Test/WiFi_LoRa_32FactoryTest/WiFi_LoRa_32FactoryTest.ino
 * 
 * Low_POwer source code.
 * https://github.com/HelTecAutomation/Heltec_ESP32/blob/master/examples/Low_Power/Low_Power.ino
 * 
 *************************************************************/

#include "Arduino.h"
#include "heltec.h"
#include "images.h"
#include "painlessMesh.h"

// painlessmesh constants
const String MESH_PREFIX = "AutoMeshWLAN";
const String MESH_PASSWORD = "password";
const unsigned int MESH_PORT = 5555;

// Heltec constants
const unsigned long RF_BAND = 0x915E6; /* LoRa frequency used in Australia is 915 megahertz. */
const bool ENABLE_OLED = true;
const bool ENABLE_LORA = true;
const bool ENABLE_SERIAL = true;
const bool ENABLE_LORA_PABOOST = true;

// sleep timer constants
const unsigned int MICROSECONDS_TO_SECONDS = 1000000; /* Convert microseconds to seconds. */
const unsigned int SECONDS_TO_MINUTES = 60;           /* Convert seconds to minutes. */
const unsigned long SLEEP_TIME_DELAY = 30;            /* Number of seconds to delay deep sleep. */

// message delay constant
const unsigned long DELAY_MILLIS = 1000; /* Time in milliseconds between messages to OLED and serial output. */

RTC_DATA_ATTR int bootCount = 0; // track the number of reboots from deep sleep

bool otherNodesConnected = false; // track if there are other nodes connected to the mesh

unsigned long sleepTime = 0; //

Scheduler taskScheduler; // scheduler for the taskSendMessage task
painlessMesh  mesh;      // mesh object to control all the mesh settings

// this is so the Task declaration doesn't cause a compile error
void sendMessage();

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage ); // task used to run sendMessage()

// For displaying the Heltec logo.
void logo(){
  Heltec.display -> clear();
  Heltec.display -> drawXbm(0, 5, logo_width, logo_height, (const unsigned char *)logo_bits);
  Heltec.display -> display();
  delay(DELAY_MILLIS);
}

// For creating traffic on the mesh network.
void sendMessage() {
  
  String msg = "Message from node ";
  msg += mesh.getNodeId();
  mesh.sendBroadcast( msg );
  taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

// Display messages on the OLED and via the serial connection.
void printToSerialAndOLED(String message) {

  // width of the Heltec OLED screen
  uint16_t oledWidth = Heltec.display -> getWidth();
  
  // Display the message on multiple lines if it is longer than the OLED screen.
  Heltec.display -> clear();
  Heltec.display -> drawStringMaxWidth(0, 0, oledWidth, message);
  Heltec.display -> display();

  // Send the message to serial output as well.
  Serial.println(message);

  // Wait one second for the user to read the message on the OLED screen.
  unsigned long nowMillis = millis();
  while (millis() < nowMillis + DELAY_MILLIS) {
    // wait for one second
  }
}

// return how many seconds it has been since the last reboot
unsigned long uptime() {
  return millis() / 1000;
}

// delay deep sleep for another 30 seconds
void delaySleep() {
  sleepTime = uptime() + SLEEP_TIME_DELAY;
}

// What this board does when a message is sent to it.
// Needed for painlessMesh library.
void receivedCallback( uint32_t from, String &msg ) {

  // Display messages addressed to this node.
  String message = "Message: " + msg + " received from node ID" + String(from);
  printToSerialAndOLED(message);
}

// What this board does when a new connection is made.
// Needed for painlessMesh library.
void newConnectionCallback(uint32_t nodeId) {
  
  otherNodesConnected = true;
  String message = "New connection made from node ID " + String(nodeId);
  printToSerialAndOLED(message);
}

// What this board does when mesh connections change.
// Needed for painlessMesh library.
void changedConnectionCallback() {

  // check how many nodes are connected
  int numberOfNodes = mesh.getNodeList().size();
  String message = "Mesh connections have changed. Now there are " + String(numberOfNodes) + " nodes connected.";
  printToSerialAndOLED(message);

  // Check if there are any other nodes connected to the mesh.
  otherNodesConnected = numberOfNodes >= 1;

  // record how many seconds have passed since the last reboot if there are no other nodes connected
  if (!otherNodesConnected) {
    delaySleep();
  }
}

// What this board does when clocks are resynchronised.
// Needed for painlessMesh library.
void nodeTimeAdjustedCallback(int32_t offset) {
  String message = "Time adjusted to " + String(mesh.getNodeTime()) + " and offset changed to " + String(offset);
  printToSerialAndOLED(message);
}

// Display the number of times this board has rebooted.
void printBootCount() {
  bootCount++;
  String message = "Boot count: " + String(bootCount);
  printToSerialAndOLED(message);
}

// Display why ESP32 was awoken from deep sleep.
void printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
  String message = "printWakeupReason() has no message to print!";

  switch(wakeupReason) {
    case 1:
      message = "Wakeup caused by external signal using RTC_IO.";
      break;
    case 2:
      message = "Wakeup caused by external signal using RTC_CNTL.";
      break;
    case 3:
      message = "Wakeup caused by touchpad.";
      break;
    case 4:
      message = "Wakeup caused by timer.";
      break;
    case 5:
      message = "Wakeup caused by ULP program.";
      break;
    default:
      message = "This board has just powered on, it didn't wake up from deep sleep.";
      break;
  }
  printToSerialAndOLED(message);
}

// Send the device to sleep for a few minutes.
void deepSleep(unsigned int sleepTime) {
    
  String message = "ESP32 will now sleep for " + String(sleepTime) + " minutes";
  printToSerialAndOLED(message);
  
  LoRa.end();
  LoRa.sleep();
  delay(100);

  pinMode(5,INPUT);
  pinMode(14,INPUT);
  pinMode(15,INPUT);
  pinMode(16,INPUT);
  pinMode(17,INPUT);
  pinMode(18,INPUT);
  pinMode(19,INPUT);
  pinMode(26,INPUT);
  pinMode(27,INPUT);
  delay(100);

  message = "Going to sleep now";
  printToSerialAndOLED(message);
  
  esp_sleep_enable_timer_wakeup(sleepTime * MICROSECONDS_TO_SECONDS * SECONDS_TO_MINUTES);
  esp_deep_sleep_start();
}

// Runs once when the device boots up.
void setup() {

  // Initialise Heltec board: enable the OLED screen, enable LoRa, enable serial output
  // and set the RF band to 915 megahertz.
  Heltec.begin(ENABLE_OLED, ENABLE_LORA, ENABLE_SERIAL, ENABLE_LORA_PABOOST, RF_BAND);
  
  // display the Heltect logo
  logo();

  printBootCount();
  printWakeupReason();
  printToSerialAndOLED("Starting mesh network and waiting for other nodes to connect.");
  delaySleep();

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &taskScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  taskScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
}

// Repeats for as long as the device is active/not sleeping.
void loop() {
  // Handle general mesh administration tasks.
  // This will run the user scheduler as well.
  mesh.update();

  // if the there are no nodes connected to the mesh for 30 seconds,
  // go to sleep for 2 minutes.
  if (!otherNodesConnected && uptime() > sleepTime) {
    deepSleep(2);
  }
}
