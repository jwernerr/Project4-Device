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

//BLE 
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLE2901.h>

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLE2901 *descriptor_2901 = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "b5c7d953-a1f2-4893-9dc2-3572aa08fe2b"
#define CHARACTERISTIC_UUID "0633a6af-6820-4154-b2bf-2dc8d8c51f02"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
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
int wave_counter = 0;

void setup() {
  Serial.begin(115200);

  while (!Serial){
    delay(10);
  }
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
  BLEDevice::init("AAAA");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE
  );

  // Creates BLE Descriptor 0x2902: Client Characteristic Configuration Descriptor (CCCD)
  // Descriptor 2902 is not required when using NimBLE as it is automatically added based on the characteristic properties
  pCharacteristic->addDescriptor(new BLE2902());
  // Adds also the Characteristic User Description - 0x2901 descriptor
  descriptor_2901 = new BLE2901();
  descriptor_2901->setDescription("My own description for this characteristic.");
  descriptor_2901->setAccessPermissions(ESP_GATT_PERM_READ);  // enforce read only - default is Read|Write
  pCharacteristic->addDescriptor(descriptor_2901);

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
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

    /* Serial.print("Acceleration X: ");
    Serial.print(a.acceleration.x);
    Serial.print(", Y: ");
    Serial.print(a.acceleration.y);
    Serial.print(", Z: ");
    Serial.print(a.acceleration.z);
    Serial.println(" m/s^2"); */

//calibration correction
    g.gyro.x -= RateCalibrationGX;
    g.gyro.y -= RateCalibrationGY;
    g.gyro.z -= RateCalibrationGZ;

    /* Serial.print("Rotation X: ");
    Serial.print(g.gyro.x);
    Serial.print(", Y: ");
    Serial.print(g.gyro.y);
    Serial.print(", Z: ");
    Serial.print(g.gyro.z);
    Serial.println(" rad/s");

    Serial.print("Temperature: ");
    Serial.print(temp.temperature);
    Serial.println(" degC"); */

    //check_Waving(a.acceleration.x, a.acceleration.y, a.acceleration.z);

    //Serial.println("");
  //BLE loop
  // notify changed value
  if (deviceConnected) {
    if (check_Waving(a.acceleration.x, a.acceleration.y, a.acceleration.z)){
      value=1;
    }
    else{
      value=0;
    }
    //value=a.acceleration.x.toString()+a.acceleration.y+a.acceleration.z;
    pCharacteristic->setValue((uint8_t *)&value, 1);
    pCharacteristic->notify();
    delay(500);
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}

bool check_Waving(float aX, float aY, float aZ){
  if(aZ * aZ >= 7 * 7){
    wave_counter +=1;
  }
  else{
    wave_counter -=1;
  }
  if(wave_counter < 0){
    wave_counter = 0;
  }
  if(wave_counter >=4){
    Serial.println("Waving :D");
    wave_counter=4;
    return true;
  }
  return false;
}

