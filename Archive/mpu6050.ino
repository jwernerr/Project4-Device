// Basic demo for accelerometer readings from Adafruit MPU6050

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;


float RateCalibrationGX, RateCalibrationGY, RateCalibrationGZ;
float RateCalibrationAX, RateCalibrationAY, RateCalibrationAZ;
int RateCalibrationNumber;
int wave_counter = 0;

void setup(void) {
    Serial.begin(115200);
    while (!Serial)
        delay(10); // will pause Zero, Leonardo, etc until serial console opens

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
}

void loop() {

  /* Get new sensor events with the readings */
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

  /* Print out the values */
    a.acceleration.x -= RateCalibrationAX;
    a.acceleration.y -= RateCalibrationAY;
    a.acceleration.z -= RateCalibrationAZ;

    Serial.print("Acceleration X: ");
    Serial.print(a.acceleration.x);
    Serial.print(", Y: ");
    Serial.print(a.acceleration.y);
    Serial.print(", Z: ");
    Serial.print(a.acceleration.z);
    Serial.println(" m/s^2");

//calibration correction
    g.gyro.x -= RateCalibrationGX;
    g.gyro.y -= RateCalibrationGY;
    g.gyro.z -= RateCalibrationGZ;

    Serial.print("Rotation X: ");
    Serial.print(g.gyro.x);
    Serial.print(", Y: ");
    Serial.print(g.gyro.y);
    Serial.print(", Z: ");
    Serial.print(g.gyro.z);
    Serial.println(" rad/s");

    Serial.print("Temperature: ");
    Serial.print(temp.temperature);
    Serial.println(" degC");

    check_Waving(a.acceleration.x, a.acceleration.y, a.acceleration.z);

    Serial.println("");
    delay(1000);
}



void check_Waving(float aX, float aY, float aZ){
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
    }
    Serial.println(wave_counter);
}
