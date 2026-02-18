/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect handler associated with the server starts a background task that performs notification
   every couple of seconds.
*/
//Device information
String deviceID=String("d1234560"); //X 000000 I

int vibPin=15;
int vibTimer=0;
bool vibState=false;

int ledPin=18;
int ledTimer=0;
bool ledState=false;

//#include <Arduino_JSON.h>

//BLE 
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLE2901.h>

BLEServer *pServer = NULL;

BLECharacteristic *pCharacteristicAcc = NULL;
BLECharacteristic *pCharacteristicRot = NULL;
BLECharacteristic *pCharacteristicTemp = NULL; //not needed but the data exists so whatever

BLECharacteristic *pCharacteristicID = NULL;

BLECharacteristic *pCharacteristicLED = NULL;
BLECharacteristic *pCharacteristicVib = NULL;

BLE2901 *descriptor_2901 = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

float ValueTemp;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID_IMU        "e9025a47-2b1d-44fe-9d51-939cdd68e0fa"
#define CHARACTERISTIC_UUID_ACC     "4c32fea1-2fd8-4978-8ca9-2dd6bdb3a37c"
#define CHARACTERISTIC_UUID_ROT     "9969f710-a2bf-4842-9f68-1a08f082ff4d"
#define CHARACTERISTIC_UUID_TEMP    "f251c49e-2019-4f8e-915d-b157ed46f2c9"

#define SERVICE_UUID_USERDATA   "e72accb5-3e32-4a9f-96f9-57aa8f62043e"
#define CHARACTERISTIC_UUID_ID      "45d3196c-58c7-4702-b559-e09bdbdfd264"

#define SERVICE_UUID_OUTPUT     "9925cdf1-edc8-46e9-bdde-66eb0dae208b"
#define CHARACTERISTIC_UUID_LED     "9681f19b-e769-4343-9d72-8d959e4caa32"
#define CHARACTERISTIC_UUID_VIB     "60599b1b-083e-4386-a682-160558733e0a"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

class OutputCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristicLED) {
    String value = pCharacteristicLED->getValue();

    if (value.length() > 0) {
      Serial.println("*********");
      Serial.print("New value: ");
      for (int i = 0; i < value.length(); i++) {
        Serial.print(value[i]);
      }

      Serial.println();
      Serial.println("*********");
    }
  }
};

//MPU6050
// Basic demo for accelerometer readings from Adafruit MPU6050

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;


float RateCalibrationGX, RateCalibrationGY, RateCalibrationGZ;
float RateCalibrationAX, RateCalibrationAY, RateCalibrationAZ;
int RateCalibrationNumber;

void setup() {
  Serial.begin(115200);

  while (!Serial){
    delay(10);
  }

  pinMode(ledPin, OUTPUT);
  pinMode(vibPin, OUTPUT);
  //MPU6050 Setup
  Serial.println("Adafruit MPU6050 test!");
  // Try to initialize!
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) {
            delay(10);
        }
    }
    Serial.println("MPU6050 Found!");

    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    Serial.print("Accelerometer range set to: ");
    switch (mpu.getAccelerometerRange()) {
        case MPU6050_RANGE_2_G:
            Serial.println("+-2G");
            break;
        case MPU6050_RANGE_4_G:
            Serial.println("+-4G");
            break;
        case MPU6050_RANGE_8_G:
            Serial.println("+-8G");
            break;
        case MPU6050_RANGE_16_G:
            Serial.println("+-16G");
            break;
    }
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    Serial.print("Gyro range set to: ");
    switch (mpu.getGyroRange()) {
        case MPU6050_RANGE_250_DEG:
            Serial.println("+- 250 deg/s");
            break;
        case MPU6050_RANGE_500_DEG:
            Serial.println("+- 500 deg/s");
            break;
        case MPU6050_RANGE_1000_DEG:
            Serial.println("+- 1000 deg/s");
            break;
        case MPU6050_RANGE_2000_DEG:
            Serial.println("+- 2000 deg/s");
            break;
    }

    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.print("Filter bandwidth set to: ");
    switch (mpu.getFilterBandwidth()) {
        case MPU6050_BAND_260_HZ:
            Serial.println("260 Hz");
            break;
        case MPU6050_BAND_184_HZ:
            Serial.println("184 Hz");
            break;
        case MPU6050_BAND_94_HZ:
            Serial.println("94 Hz");
            break;
        case MPU6050_BAND_44_HZ:
            Serial.println("44 Hz");
            break;
        case MPU6050_BAND_21_HZ:
            Serial.println("21 Hz");
            break;
        case MPU6050_BAND_10_HZ:
            Serial.println("10 Hz");
            break;
        case MPU6050_BAND_5_HZ:
            Serial.println("5 Hz");
            break;
    }

    Serial.println("Calibrating gyro...");
    sensors_event_t a, g, temp;
    for(RateCalibrationNumber = 0; RateCalibrationNumber<2000; RateCalibrationNumber ++){
        mpu.getEvent(&a, &g, &temp);
        RateCalibrationGX += g.gyro.x;
        RateCalibrationGY += g.gyro.y;
        RateCalibrationGZ += g.gyro.z;

        RateCalibrationAX += a.acceleration.x;
        RateCalibrationAY += a.acceleration.y;
        RateCalibrationAZ += a.acceleration.z;
        delay(5);
    }

    Serial.println("Calibration finished");
    RateCalibrationGX /= 2000;
    RateCalibrationGY /= 2000;
    RateCalibrationGZ /= 2000;

    RateCalibrationAX /= 2000;
    RateCalibrationAY /= 2000;
    RateCalibrationAZ /= 2000;

    Serial.println("");
    delay(100);

  //BLE Server setup
  

  // Create the BLE Device
  BLEDevice::init("FTP");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Services
  BLEService *pServiceIMU = pServer->createService(SERVICE_UUID_IMU);
  BLEService *pServiceUserData = pServer->createService(SERVICE_UUID_USERDATA);
  BLEService *pServiceOutput = pServer->createService(SERVICE_UUID_OUTPUT);

  // Create BLE Characteristics
  pCharacteristicAcc = pServiceIMU->createCharacteristic(
    CHARACTERISTIC_UUID_ACC,
    BLECharacteristic::PROPERTY_READ |  BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE
  );
  pCharacteristicRot = pServiceIMU->createCharacteristic(
    CHARACTERISTIC_UUID_ROT,
    BLECharacteristic::PROPERTY_READ |  BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE
  );
  pCharacteristicTemp = pServiceIMU->createCharacteristic(
    CHARACTERISTIC_UUID_TEMP,
    BLECharacteristic::PROPERTY_READ |  BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE
  );

  pCharacteristicID = pServiceUserData->createCharacteristic(
    CHARACTERISTIC_UUID_ID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_INDICATE
  );

  pCharacteristicLED = pServiceOutput->createCharacteristic(
    CHARACTERISTIC_UUID_LED,
    BLECharacteristic::BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristicVib = pServiceOutput->createCharacteristic(
    CHARACTERISTIC_UUID_VIB,
    BLECharacteristic::BLECharacteristic::PROPERTY_WRITE
  );

  

  // Creates BLE Descriptor 0x2902: Client Characteristic Configuration Descriptor (CCCD)
  // Descriptor 2902 is not required when using NimBLE as it is automatically added based on the characteristic properties
  pCharacteristicAcc->addDescriptor(new BLE2902());
  // Adds also the Characteristic User Description - 0x2901 descriptor
  descriptor_2901 = new BLE2901();
  descriptor_2901->setDescription("Acceleration x,y,z");
  descriptor_2901->setAccessPermissions(ESP_GATT_PERM_READ);  // enforce read only - default is Read|Write
  pCharacteristicAcc->addDescriptor(descriptor_2901);

  pCharacteristicRot->addDescriptor(new BLE2902());
  pCharacteristicRot->addDescriptor(descriptor_2901);

  // Start the service
  pServiceIMU->start();
  pServiceUserData->start();
  pServiceOutput->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

  pAdvertising->addServiceUUID(SERVICE_UUID_IMU);
  pAdvertising->addServiceUUID(SERVICE_UUID_USERDATA);
  pAdvertising->addServiceUUID(SERVICE_UUID_OUTPUT);

  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

  pCharacteristicID->setValue((uint8_t *)&deviceID, sizeof(deviceID));

  pCharacteristicVib->setValue((uint8_t *)0,2);
  pCharacteristicVib->setCallbacks(new OutputCallbacks());

  pCharacteristicLED->setValue((uint8_t *)0,2);
  pCharacteristicLED->setCallbacks(new OutputCallbacks());

}

void loop() {
  //MPU6050 loop
  /* Get new sensor events with the readings */
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

  /* Print out the values */
    a.acceleration.x -= RateCalibrationAX;
    a.acceleration.y -= RateCalibrationAY;
    a.acceleration.z -= RateCalibrationAZ;

//calibration correction
    g.gyro.x -= RateCalibrationGX;
    g.gyro.y -= RateCalibrationGY;
    g.gyro.z -= RateCalibrationGZ;

  //BLE loop
  // notify changed value
  if (deviceConnected) {
    handleOutputs();
    
    float valuesAcc[3]={a.acceleration.x,a.acceleration.y,a.acceleration.z};
    pCharacteristicAcc->setValue((uint8_t *)valuesAcc, sizeof(valuesAcc));

    float valuesRot[3]={g.gyro.x,g.gyro.y,g.gyro.z};
    pCharacteristicRot->setValue((uint8_t *)valuesRot, sizeof(valuesRot));

    pCharacteristicTemp->setValue((uint8_t *)&temp.temperature, sizeof(temp.temperature));
    
    pCharacteristicAcc->notify();
    pCharacteristicRot->notify();
    //Serial.println(pCharacteristicAcc->getValue().toInt());
    delay(500);
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
    pCharacteristicLED->setValue(0,2);
    pCharacteristicVib->setValue(0,2);
    handleOutputs();
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}



void handleOutputs(){
  switch (pCharacteristicVib->getValue().toInt()){
    case 0:
      if (vibState){
        digitalWrite(vibPin, LOW);
        vibState=false;
      }
      break;
    case 1:
      if (!vibState){
        digitalWrite(vibPin, HIGH);
        vibState=true;
      }
      break;
    case 2:
      vibTimer--;
      if (vibTimer<0 && vibState){
        analogWrite(vibPin, 0);
        vibState=false;
        vibTimer=10;
      }
      else if(vibTimer<0 && !vibState){
        analogWrite(vibPin, 150);
        vibState=true;
        vibTimer=1;
      }
      break;
    default:
      if (vibState){
        digitalWrite(vibPin, LOW);
        vibState=false;
      }
      Serial.println("default vib");
  }

  switch (pCharacteristicLED->getValue().toInt()){
    case 0:
      if (ledState){
        digitalWrite(ledPin, LOW);
        ledState=false;
      }
      Serial.println("case0");
      break;
    case 1:
      if (!ledState){
        digitalWrite(ledPin, HIGH);
        ledState=true;
      }
      Serial.println("case1");
      break;
    case 2:
      ledTimer--;
      if (ledTimer<0){
        if (ledState){
          digitalWrite(ledPin, LOW);
          ledState=false;
        }
        else{
          digitalWrite(ledPin, HIGH);
          ledState=true;
        }
        ledTimer=1;
      }
      break;
    default:
      if (ledState){
        digitalWrite(ledPin, LOW);
        ledState=false;
      }
      Serial.println("default");
  }
}

//artifacts, probably not needed

/* String jsonCreateArray3(float x, float y, float z){
  JSONVar newArray;
  newArray[0]=x;
  newArray[1]=y;
  newArray[2]=z;

  String jsonString=JSON.stringify(newArray);
  Serial.println(newArray);
  return jsonString;
} */


