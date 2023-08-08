/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#include "BLEDevice.h"
//#include "BLEScan.h"


//const char* targetMacAddress = "A4:34:F1:7F:9B:8B";// Replace with your MAC address or set to nullptr if you want to scan.
//const char* targetMacAddress = "A4:34:F1:7F:DA:DD"; // Replace with your MAC address or set to nullptr if you want to scan.

// List of MAC addresses for the sensors
const char* sensorMacAddresses[] = {
    "A4:34:F1:7F:9B:8B",  // MAC address of the first sensor
    "A4:34:F1:7F:DA:DD",  // MAC address of the second sensor
    // ... add more MAC addresses as needed
};

const int numSensors = sizeof(sensorMacAddresses) / sizeof(sensorMacAddresses[0]);
int currentSensorIndex = 0;

//const char* targetMacAddress = nullptr;

// UUIDs
struct SensorUUIDs {
    static BLEUUID service;
    static BLEUUID mac;
    static BLEUUID bat;
    static BLEUUID temp;
    static BLEUUID hum;
    static BLEUUID bar;
    static BLEUUID led;
    //bluetooth advertising frequency//
    static BLEUUID rate;
};

BLEUUID SensorUUIDs::service("EF090000-11D6-42BA-93B8-9DD7EC090AB0");
BLEUUID SensorUUIDs::mac("EF09000D-11D6-42BA-93B8-9DD7EC090AA9");
BLEUUID SensorUUIDs::bat("EF090007-11D6-42BA-93B8-9DD7EC090AA9");
BLEUUID SensorUUIDs::temp("EF090080-11D6-42BA-93B8-9DD7EC090AA9");
BLEUUID SensorUUIDs::hum("EF090081-11D6-42BA-93B8-9DD7EC090AA9");
BLEUUID SensorUUIDs::bar("EF090082-11D6-42BA-93B8-9DD7EC090AA9");
BLEUUID SensorUUIDs::led("EF09000C-11D6-42BA-93B8-9DD7EC090AA9");
BLEUUID SensorUUIDs::rate("EF090005-11D6-42BA-93B8-9DD7EC090AA9");

// Characteristics
struct SensorCharacteristics {
  //6 byte int8[6]
  BLERemoteCharacteristic* mac;
  //4 byte int16[2]
  BLERemoteCharacteristic* bat;
  //4 byte int32
  BLERemoteCharacteristic* temp;
  //4 byte int32
  BLERemoteCharacteristic* hum;
  //4 byte int32
  BLERemoteCharacteristic* bar;
  //1 byte int8
  BLERemoteCharacteristic* led; 
  //1 byte int8
  BLERemoteCharacteristic* rate; 
};

SensorCharacteristics sensorChars;


static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

static BLEAdvertisedDevice* myDevice;
BLEClient* pClient = nullptr;


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.write(pData, length);
    Serial.println();
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};


bool macStringToByteArray(const char* macStr, uint8_t* macArray) {
    int values[6];
    if (6 == sscanf(macStr, "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5])) {
        for (int i = 0; i < 6; ++i) {
            macArray[i] = (uint8_t)values[i];
        }
        return true;
    }
    return false;
}


bool connectToServer(uint8_t* macArray) {
    //Serial.print("Forming a connection to ");
    Serial.println(sensorMacAddresses[currentSensorIndex]);

    // Use the provided macArray to create a BLEAddress object
    BLEAddress pAddress(macArray);

    // Connect to the remove BLE Server

    if (pClient == nullptr) {
        pClient = BLEDevice::createClient();
        //Serial.println(" - Created client");
    }
    
    // Set the address we want to connect to; it should be a public address
    //pClient->setAddressType(BLE_ADDR_TYPE_PUBLIC);

    // Connect to the remove BLE Server
    pClient->connect(pAddress);
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server
    BLERemoteService* pRemoteService = pClient->getService(SensorUUIDs::service);
    if (pRemoteService == nullptr) {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(SensorUUIDs::service.toString().c_str());
        pClient->disconnect();
        return false;
    }
    //Serial.println(" - Found our service");

    // Obtain references to the characteristics in the service of the remote BLE server
    sensorChars.mac = pRemoteService->getCharacteristic(SensorUUIDs::mac);
    sensorChars.bat = pRemoteService->getCharacteristic(SensorUUIDs::bat);
    sensorChars.temp = pRemoteService->getCharacteristic(SensorUUIDs::temp);
    sensorChars.hum = pRemoteService->getCharacteristic(SensorUUIDs::hum);
    sensorChars.bar = pRemoteService->getCharacteristic(SensorUUIDs::bar);
    sensorChars.led = pRemoteService->getCharacteristic(SensorUUIDs::led);
    sensorChars.rate = pRemoteService->getCharacteristic(SensorUUIDs::rate);

    if (sensorChars.mac == nullptr || sensorChars.bat == nullptr || sensorChars.temp == nullptr || sensorChars.hum == nullptr || sensorChars.bar == nullptr) {
        Serial.println("Failed to find our characteristics!");
        pClient->disconnect();
        return false;
    }
    //Serial.println(" - Found our characteristics");

    // Perform read and write operations on the characteristics
    // ... your existing code ...

    return true;
}


/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(SensorUUIDs::service)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  //Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

    doConnect = true;
}

void printInt32Value(const std::string& value) {
  if (value.length() == 4) {
    int32_t intValue;
    memcpy(&intValue, value.data(), sizeof(int32_t));
    Serial.println(intValue);
  } else {
    Serial.println("Invalid int32 value length!");
  }
}

void printInt16Value(const std::string& value) {
  if (value.length() == 4) {
    int16_t intValue;
    memcpy(&intValue, value.data(), sizeof(int16_t));
    Serial.println(intValue);
  } else {
    Serial.println("Invalid int16 value length!");
  }
}


void printInt8Value(const std::string& value) {
  if (value.length() == 1) {
    int8_t intValue;
    memcpy(&intValue, value.data(), sizeof(int8_t));
    Serial.println(intValue);
  } else {
    Serial.println("Invalid int8 value length!");
  }
}

void printMacAddress(const std::string& value) {
  if (value.length() == 6) {
    char macAddress[18]; // Buffer for the MAC address string (including null-terminator).
    sprintf(macAddress, "%02X:%02X:%02X:%02X:%02X:%02X",
            value[5], value[4], value[3], value[2], value[1], value[0]);

    Serial.println(macAddress);
  } else {
    Serial.println("Invalid MAC address length!");
  }
}

// This is the Arduino main loop function.
void loop() {
        uint8_t macArray[6];
        if (macStringToByteArray(sensorMacAddresses[currentSensorIndex], macArray)) {
            if (connectToServer(macArray)) {
                Serial.println("connected");

                //write a 32 bit character to the temp characteristic to trigger a read of the temp sensor, then wait for sensor to be ready//
                int32_t newValue = 0x01000000;
                uint8_t byteArray[sizeof(int32_t)];
                memcpy(byteArray, &newValue, sizeof(int32_t));
                sensorChars.temp->writeValue(byteArray, sizeof(int32_t));
                sensorChars.hum->writeValue(byteArray, sizeof(int32_t));
                sensorChars.bar->writeValue(byteArray, sizeof(int32_t));
                delay(150);

                std::string temp = sensorChars.temp->readValue();
                std::string humidity = sensorChars.hum->readValue();
                std::string pressure = sensorChars.bar->readValue();
                std::string bat = sensorChars.bat->readValue();
                std::string mac = sensorChars.mac->readValue();
                std::string led = sensorChars.led->readValue();
                std::string rate = sensorChars.rate->readValue();

                Serial.print("mac: ");
                printMacAddress(mac);

                Serial.print("temp: ");
                printInt32Value(temp);

                Serial.print("humidity: ");
                printInt32Value(humidity);

                Serial.print("pressure: ");
                printInt32Value(pressure);      

                Serial.print("battery: ");
                printInt16Value(bat);  

                Serial.print("led: ");
                printInt8Value(led);  

                Serial.print("rate: ");
                printInt8Value(rate);  

                // After collecting data, disconnect
                pClient->disconnect();
                connected = false;

                // Move to the next sensor in the list
                currentSensorIndex = (currentSensorIndex + 1) % numSensors;
            } else {
                Serial.println("We have failed to connect to the server; there is nothing more we will do.");
            }
    }
    delay(500);
    if(currentSensorIndex + 1 == numSensors){
      Serial.println("waiting...");
      delay(20000); // Delay between loops.
    }
}
