/**
 * A BLE client example that connects to multiple BLE servers simultaneously.
 *
 * This example demonstrates how to:
 * - Scan for multiple BLE servers
 * - Connect to multiple servers at the same time
 * - Interact with characteristics on different servers
 * - Handle disconnections and reconnections
 *
 * The example looks for servers advertising the service UUID: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
 * and connects to up to MAX_SERVERS servers.
 *
 * Created by lucasssvaz
 * Based on the original Client example by Neil Kolban and chegewara
 */

#include "BLEDevice.h"

//#include <Arduino_JSON.h>

// The remote service we wish to connect to.
static BLEUUID serviceUUIDIMU("e9025a47-2b1d-44fe-9d51-939cdd68e0fa");
static BLEUUID serviceUUIDUserData("e72accb5-3e32-4a9f-96f9-57aa8f62043e");
static BLEUUID serviceUUIDOutput("9925cdf1-edc8-46e9-bdde-66eb0dae208b");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUIDAcc("4c32fea1-2fd8-4978-8ca9-2dd6bdb3a37c");
static BLEUUID charUUIDRot("9969f710-a2bf-4842-9f68-1a08f082ff4d");

static BLEUUID charUUIDID("45d3196c-58c7-4702-b559-e09bdbdfd264");

static BLEUUID charUUIDLED("9681f19b-e769-4343-9d72-8d959e4caa32");
static BLEUUID charUUIDVib("60599b1b-083e-4386-a682-160558733e0a");

struct stationData{
  bool hasStar;
  int respawnTimer;
  int ledPin;
};

stationData starMoney;


// Maximum number of servers to connect to
#define MAX_SERVERS 3

// Structure to hold information about each connected server
struct ServerConnection {
  BLEClient *pClient;
  BLEAdvertisedDevice *pDevice;

  String deviceID;

  float acc[3];
  float rot[3];

  BLERemoteService *pRemoteServiceIMU;
  BLERemoteService *pRemoteServiceUserData;
  BLERemoteService *pRemoteServiceOutput;

  BLERemoteCharacteristic *pRemoteCharacteristicAcc;
  BLERemoteCharacteristic *pRemoteCharacteristicRot;

  BLERemoteCharacteristic *pRemoteCharacteristicID;

  BLERemoteCharacteristic *pRemoteCharacteristicLED;
  BLERemoteCharacteristic *pRemoteCharacteristicVib;

  bool connected;
  bool doConnect;
  bool close;

  String name;

  int waveCounter;
};

// Array to manage multiple server connections
ServerConnection servers[MAX_SERVERS];
int connectedServers = 0;
static bool doScan = true;

// Callback function to handle notifications from any server
static void notifyCallbackAcc(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  // Find which server this notification came from
  for (int i = 0; i < MAX_SERVERS; i++) {
    if (servers[i].connected && servers[i].pRemoteCharacteristicAcc == pBLERemoteCharacteristic) {
      /* Serial.print("Notify from server ");
      Serial.print(servers[i].name);
      Serial.print(" - Characteristic: ");
      Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
      Serial.print(" | Length: ");
      Serial.print(length);
      Serial.print(" | Data: ");
      Serial.write(pData, length);
      Serial.println(); */
      float valuesAcc[3];
      memcpy(valuesAcc,pData,sizeof(valuesAcc));
      for (int j=0;j<3;j++){
        servers[i].acc[j]=valuesAcc[j];
      }
      break;
    }
  }
}

static void notifyCallbackRot(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  // Find which server this notification came from
  for (int i = 0; i < MAX_SERVERS; i++) {
    if (servers[i].connected && servers[i].pRemoteCharacteristicRot == pBLERemoteCharacteristic) {
      /* Serial.print("Notify from server ");
      Serial.print(servers[i].name);
      Serial.print(" - Characteristic: ");
      Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
      Serial.print(" | Length: ");
      Serial.print(length);
      Serial.print(" | Data: ");
      Serial.write(pData, length);
      Serial.println(); */
      float valuesRot[3];
      memcpy(valuesRot,pData,sizeof(valuesRot));
      for (int j=0;j<3;j++){
        servers[i].rot[j]=valuesRot[j];
      }
      break;
    }
  }
}

// Client callback class to handle connect/disconnect events
class MyClientCallback : public BLEClientCallbacks {
  int serverIndex;

public:
  MyClientCallback(int index) : serverIndex(index) {}

  void onConnect(BLEClient *pclient) {
    Serial.print("Connected to server ");
    Serial.println(servers[serverIndex].name);

    if (servers[serverIndex].connected && servers[serverIndex].pRemoteCharacteristicLED != nullptr) {
        // Create a unique message for each server
        String newValue = "0";

        Serial.print("Sending to ");
        Serial.print(servers[serverIndex].name);
        Serial.print(": ");
        Serial.println(newValue);

        // Write the value to the characteristic
        servers[serverIndex].pRemoteCharacteristicLED->writeValue(newValue.c_str(), newValue.length());
    }
  }

  void onDisconnect(BLEClient *pclient) {
    servers[serverIndex].connected = false;
    connectedServers--;
    Serial.print("Disconnected from server ");
    Serial.print(servers[serverIndex].name);
    Serial.print(" | Total connected: ");
    Serial.println(connectedServers);
    doScan = true;  // Resume scanning to find replacement servers
  }
};

// Function to connect to a specific server
bool connectToServer(int serverIndex) {
  Serial.print("Connecting to server ");
  Serial.print(serverIndex);
  Serial.print(" at address: ");
  Serial.println(servers[serverIndex].pDevice->getAddress().toString().c_str());

  servers[serverIndex].pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  // Set the callback for this specific server connection
  servers[serverIndex].pClient->setClientCallbacks(new MyClientCallback(serverIndex));

  // Connect to the remote BLE Server
  servers[serverIndex].pClient->connect(servers[serverIndex].pDevice);
  Serial.println(" - Connected to server");
  servers[serverIndex].pClient->setMTU(517);  // Request maximum MTU from server

  // Obtain a reference to the service we are after in the remote BLE server
  servers[serverIndex].pRemoteServiceIMU = servers[serverIndex].pClient->getService(serviceUUIDIMU);
  servers[serverIndex].pRemoteServiceUserData = servers[serverIndex].pClient->getService(serviceUUIDUserData);
  servers[serverIndex].pRemoteServiceOutput = servers[serverIndex].pClient->getService(serviceUUIDOutput);

  if (servers[serverIndex].pRemoteServiceIMU && servers[serverIndex].pRemoteServiceUserData&&servers[serverIndex].pRemoteServiceOutput){
    Serial.println("all remote services found");

    servers[serverIndex].pRemoteCharacteristicAcc = servers[serverIndex].pRemoteServiceIMU->getCharacteristic(charUUIDAcc);
    servers[serverIndex].pRemoteCharacteristicRot = servers[serverIndex].pRemoteServiceIMU->getCharacteristic(charUUIDRot);

    servers[serverIndex].pRemoteCharacteristicID = servers[serverIndex].pRemoteServiceUserData->getCharacteristic(charUUIDID);

    servers[serverIndex].pRemoteCharacteristicLED = servers[serverIndex].pRemoteServiceOutput->getCharacteristic(charUUIDLED);
    servers[serverIndex].pRemoteCharacteristicVib = servers[serverIndex].pRemoteServiceOutput->getCharacteristic(charUUIDVib);
    
    if (!servers[serverIndex].pRemoteCharacteristicAcc || !servers[serverIndex].pRemoteCharacteristicRot ||!servers[serverIndex].pRemoteCharacteristicID ||!servers[serverIndex].pRemoteCharacteristicLED ||!servers[serverIndex].pRemoteCharacteristicVib){
      servers[serverIndex].pClient->disconnect();
      return false;
    }
  }
  else{
    servers[serverIndex].pClient->disconnect();
    return false;
  }

  if (servers[serverIndex].pRemoteCharacteristicID->canRead()) {
    servers[serverIndex].deviceID = servers[serverIndex].pRemoteCharacteristicID->readValue();
    Serial.print("Initial characteristic value: ");
    Serial.println(servers[serverIndex].deviceID.c_str());
  }
  
  registerForNotifications(servers[serverIndex].pRemoteCharacteristicAcc, "Acc");
  registerForNotifications(servers[serverIndex].pRemoteCharacteristicRot, "Rot");

  servers[serverIndex].connected = true;
  connectedServers++;
  Serial.print("Successfully connected! Total servers connected: ");
  Serial.println(connectedServers);

  servers[serverIndex].waveCounter=0;

  return true;
}

// Scan callback class to find BLE servers
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // Check if this device has the service we're looking for
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUIDIMU)) {
      Serial.println(" -> This device has our service!");

      // Check if we already know about this device
      String deviceAddress = advertisedDevice.getAddress().toString().c_str();
      bool alreadyKnown = false;

      for (int i = 0; i < MAX_SERVERS; i++) {
        if (servers[i].pDevice != nullptr) {
          if (servers[i].pDevice->getAddress().toString() == deviceAddress) {
            alreadyKnown = true;
            break;
          }
        }
      }

      if (alreadyKnown) {
        Serial.println(" -> Already connected or connecting to this device");
        return;
      }

      // Find an empty slot for this server
      for (int i = 0; i < MAX_SERVERS; i++) {
        if (servers[i].pDevice == nullptr || (!servers[i].connected && !servers[i].doConnect)) {
          servers[i].pDevice = new BLEAdvertisedDevice(advertisedDevice);
          servers[i].doConnect = true;
          servers[i].name = "Server_" + String(i);
          Serial.print(" -> Assigned to slot ");
          Serial.println(i);

          // If we've found enough servers, stop scanning
          int pendingConnections = 0;
          for (int j = 0; j < MAX_SERVERS; j++) {
            if (servers[j].connected || servers[j].doConnect) {
              pendingConnections++;
            }
          }
          if (pendingConnections >= MAX_SERVERS) {
            Serial.println("Found enough servers, stopping scan");
            BLEDevice::getScan()->stop();
            doScan = false;
          }
          break;
        }
      }
    }
  }
};

void setup() {
  starMoney.hasStar=true;
  starMoney.respawnTimer=30;
  starMoney.ledPin=12;

  Serial.begin(115200);

  pinMode(starMoney.ledPin,OUTPUT);
  digitalWrite(starMoney.ledPin,HIGH);
  Serial.println("=================================");
  Serial.println("BLE Multi-Client Example");
  Serial.println("=================================");
  Serial.print("Max servers to connect: ");
  Serial.println(MAX_SERVERS);
  Serial.println();

  // Initialize all server connections
  for (int i = 0; i < MAX_SERVERS; i++) {
    servers[i].pClient = nullptr;
    servers[i].pDevice = nullptr;
    servers[i].pRemoteCharacteristicAcc = nullptr;
    servers[i].connected = false;
    servers[i].doConnect = false;
    servers[i].name = "";
  }

  // Initialize BLE
  BLEDevice::init("ESP32_MultiClient");

  // Set up BLE scanner
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

  Serial.println("Scanning for BLE servers...");
}

void loop() {
  

  if (connectedServers > 0) {
    for (int i = 0; i < MAX_SERVERS; i++) {
      if (servers[i].pClient!=nullptr){
        servers[i].close=isClose(servers[i]);
      }
      Serial.print("is close: ");
      Serial.println(servers[i].close);

      //if (servers[i].connected&&
      Serial.print("wave: ");
      Serial.println(checkWaving(servers[i].acc[0], servers[i].acc[1], servers[i].acc[2],i));
      if (servers[i].close && checkWaving(servers[i].acc[0], servers[i].acc[1], servers[i].acc[2],i)){
        starMoney.hasStar=false;
        if (servers[i].connected && servers[i].pRemoteCharacteristicLED != nullptr) {
        // Create a unique message for each server
          String newValue = "1";

          Serial.print("Sending to ");
          Serial.print(servers[i].name);
          Serial.print(": ");
          Serial.println(newValue);

        // Write the value to the characteristic
          servers[i].pRemoteCharacteristicLED->writeValue(newValue.c_str(), newValue.length());
        }
      }
      updateStation();
    }
  } else {
    Serial.println("No servers connected");
  }

  // Resume scanning if we have room for more connections
  if (doScan && connectedServers < MAX_SERVERS) {
    scanForServers();
  }
  
  delay(2000);  // Delay between loop iterations
}

void registerForNotifications(BLERemoteCharacteristic * pRemoteCharacteristic, String type){
  // Read the value of the characteristic

  // Register for notifications if available
  if (pRemoteCharacteristic->canNotify()) {
    if (type=="Acc"){
      pRemoteCharacteristic->registerForNotify(notifyCallbackAcc);
    }
    else{
      pRemoteCharacteristic->registerForNotify(notifyCallbackRot);
    }
    Serial.print(" - Registered for notifications for UUID ");
    Serial.println(pRemoteCharacteristic->getUUID().toString());
  }
}

bool isClose(ServerConnection server){
  int rssi=server.pClient->getRssi();
  Serial.print("rssi");
  Serial.println(rssi);

  int rssiCap=-50-connectedServers*10; //smaller cap the less people are around
  String newValue;

  if (rssi>rssiCap){
    server.close=true;
    newValue="2";
  }
  else{
    server.close=false;
    newValue="0";
  }
  if (server.connected && server.pRemoteCharacteristicVib != nullptr) {
    server.pRemoteCharacteristicVib->writeValue(newValue.c_str(), newValue.length());
  }
  return server.close;
}

bool checkWaving(float aX, float aY, float aZ, int serverIndex){
  Serial.print("X: ");
  Serial.print(aX);
  Serial.print(", Y: ");
  Serial.print(aY);
  Serial.print(", Z: ");
  Serial.println(aZ);
  if(aZ * aZ >= 7 * 7){
    servers[serverIndex].waveCounter +=1;
    Serial.println("waving up");
  }
  else{
    servers[serverIndex].waveCounter -=1;
  }
  if(servers[serverIndex].waveCounter < 0){
    servers[serverIndex].waveCounter = 0;
  }
  if(servers[serverIndex].waveCounter >=2){
    Serial.println("WAVE");
    servers[serverIndex].waveCounter=2;
    return true;
  }
  return false;
}

//artifacts

/* JSONVar JSONParse(uint8_t * input, size_t length){ //used AI for debugging here
  char jsonBuffer[length+1];
  memcpy(jsonBuffer,input,length);
  jsonBuffer[length]='\0';

  JSONVar parsedObject=JSON.parse(jsonBuffer);
  Serial.println(JSON.typeof_(parsedObject));
  return parsedObject;
}  */

void updateStation(){
  if (starMoney.respawnTimer<0){
    starMoney.respawnTimer=30;
    starMoney.hasStar=true;
    doScan=true;
  }

  if (starMoney.hasStar){
    analogWrite(starMoney.ledPin,random(20,256));
    //digitalWrite(starMoney.ledPin,HIGH);
  }
  else{
    analogWrite(starMoney.ledPin,0);
    starMoney.respawnTimer--;
    Serial.print("cooldown");
    Serial.println(starMoney.respawnTimer);
  }
}

void processPendingConnections(){
  // Process any pending connections
  for (int i = 0; i < MAX_SERVERS; i++) {
    if (servers[i].doConnect) {
      if (connectToServer(i)) {
        Serial.println("Connection successful");
      } else {
        Serial.println("Connection failed");
        // Clear this slot so we can try another server
        delete servers[i].pDevice;
        servers[i].pDevice = nullptr;
      }
      servers[i].doConnect = false;
    }
  }
}

void scanForServers(){
  Serial.println("Resuming scan for more servers...");
  BLEDevice::getScan()->start(5, false);
  doScan = false;
  delay(5000);  // Wait for scan to complete

  processPendingConnections();

  if (connectedServers==0 || !starMoney.hasStar){
    doScan=true;
    if (!starMoney.hasStar){
      starMoney.respawnTimer=starMoney.respawnTimer-7;
      updateStation();
    }
    delay(3000);
  }
}
