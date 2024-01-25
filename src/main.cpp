#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// TODO: add new global variables for your sensor readings and processed data
// Define the HC-SR04 sensor pins
const int triggerPin = 9; // TODO: set to your trigger pin number
const int echoPin = 8;   // TODO: set to your echo pin number

// Define the number of samples for the moving average filter
const int numSamples = 10;
int sampleIndex = 0;
float samples[numSamples];
float averageDistance = 0;

// TODO: Change the UUID to your own (any specific one works, but make sure they're different from others'). You can generate one here: https://www.uuidgenerator.net/
#define SERVICE_UUID        "7d26257b-667b-43e3-bb32-b4cfbd351479"
#define CHARACTERISTIC_UUID "eb9cea5a-5c44-45fa-80d2-7aea30b5ca9c"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

// TODO: add DSP algorithm functions here

void setup() {
    Serial.begin(9600);
    Serial.println("Starting BLE work!");

    // TODO: add codes for handling your sensor setup (pinMode, etc.)
    // Initialize the HC-SR04 sensor
    pinMode(triggerPin, OUTPUT);
    pinMode(echoPin, INPUT);
    digitalWrite(triggerPin, LOW); // Keep trigger low
    // Initialize moving average samples
    for (int i = 0; i < numSamples; i++) {
        samples[i] = 0;
    }

    // TODO: name your device to avoid conflictions
    BLEDevice::init("JJY_XIAO_ESP32S3");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Hello World");
    pService->start();
    // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    // TODO: add codes for handling your sensor readings (analogRead, etc.)
    // Trigger the HC-SR04 to start a measurement
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);

    // Measure the pulse width on the echo pin
    long duration = pulseIn(echoPin, HIGH);
    float distanceCm = duration * 0.034 / 2; // Convert to cm

    // Simple check to ensure you're getting valid readings
    if (duration == 0) {
        Serial.println("No echo received");
    } else {
        Serial.print("Raw distance: ");
        Serial.print(distanceCm);
        Serial.println(" cm");
    }

    delay(1000); // Add a delay between readings for testing
    // TODO: use your defined DSP algorithm to process the readings
    // Update the moving average with the new distance
    averageDistance -= samples[sampleIndex] / numSamples; // Subtract the oldest sample
    samples[sampleIndex] = distanceCm;
    averageDistance += samples[sampleIndex] / numSamples; // Add the new sample
    sampleIndex = (sampleIndex + 1) % numSamples;

    // Print both the raw and denoised data
    Serial.print("Raw distance: ");
    Serial.print(distanceCm);
    Serial.print(" cm, Denoised distance: ");
    Serial.print(averageDistance);
    Serial.println(" cm");
    
    // Check if BLE device is connected
    if (deviceConnected) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            // Check if denoised distance is less than 30 cm
            if (averageDistance < 30) {
                // Convert the float to a string to send via BLE
                char txBuffer[20];
                snprintf(txBuffer, sizeof(txBuffer), "%.2f cm", averageDistance);
                pCharacteristic->setValue(txBuffer);
                pCharacteristic->notify();
                Serial.print("Notify value: ");
                Serial.println(txBuffer);
            }
            previousMillis = currentMillis;
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising();  // advertise again
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}