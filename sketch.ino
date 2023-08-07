#include "BLEDevice.h"
//#include "BLEScan.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID("EF090000-11D6-42BA-93B8-9DD7EC090AB0");
// The characteristic of the remote service we are interested in.
static BLEUUID macUUID("EF09000D-11D6-42BA-93B8-9DD7EC090AA9");
static BLEUUID batUUID("EF090007-11D6-42BA-93B8-9DD7EC090AA9");
static BLEUUID tempUUID("EF090080-11D6-42BA-93B8-9DD7EC090AA9");
static BLEUUID humUUID("EF090081-11D6-42BA-93B8-9DD7EC090AA9");
static BLEUUID barUUID("EF090082-11D6-42BA-93B8-9DD7EC090AA9");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
//6 byte int8[6]
static BLERemoteCharacteristic* macCharacteristic;
//4 byte int16[2]
static BLERemoteCharacteristic* batCharacteristic;
//4 byte int32
static BLERemoteCharacteristic* tempCharacteristic;
//4 byte int32
static BLERemoteCharacteristic* humCharacteristic;
//4 byte int32
static BLERemoteCharacteristic* barCharacteristic;
static BLEAdvertisedDevice* myDevice;

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

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    macCharacteristic = pRemoteService->getCharacteristic(macUUID);
    batCharacteristic = pRemoteService->getCharacteristic(batUUID);
    tempCharacteristic = pRemoteService->getCharacteristic(tempUUID);
    humCharacteristic = pRemoteService->getCharacteristic(humUUID);
    barCharacteristic = pRemoteService->getCharacteristic(barUUID);
    if (tempCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(tempUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the temp characteristic.
    if(tempCharacteristic->canRead()) {
      std::string temp = tempCharacteristic->readValue();
      Serial.print("temperature: ");
      printInt32Value(temp);
    }

    // Read the value of the temp characteristic.
    if(humCharacteristic->canRead()) {
      std::string temp = humCharacteristic->readValue();
      Serial.print("humidity: ");
      printInt32Value(temp);
    }



    if(tempCharacteristic->canNotify())
      tempCharacteristic->registerForNotify(notifyCallback);

    connected = true;
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
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.


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

void printMacAddress(const std::string& value) {
  if (value.length() == 6) {
    char macAddress[18]; // Buffer for the MAC address string (including null-terminator).
    sprintf(macAddress, "%02X:%02X:%02X:%02X:%02X:%02X",
            value[0], value[1], value[2], value[3], value[4], value[5]);

    Serial.println(macAddress);
  } else {
    Serial.println("Invalid MAC address length!");
  }
}

// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {

    //write a 32 bit character to the temp characteristic to trigger a read of the temp sensor, then wait for sensor to be ready//
    int32_t newValue = 0x01000000;
    uint8_t byteArray[sizeof(int32_t)];
    memcpy(byteArray, &newValue, sizeof(int32_t));
    tempCharacteristic->writeValue(byteArray, sizeof(int32_t));
    humCharacteristic->writeValue(byteArray, sizeof(int32_t));
    barCharacteristic->writeValue(byteArray, sizeof(int32_t));
    delay(150);


    //std::string mac = macCharacteristic->readValue();
    //Serial.print("mac: ");
    //printMacAddress(mac);

    std::string temp = tempCharacteristic->readValue();
    std::string humidity = humCharacteristic->readValue();
    std::string pressure = barCharacteristic->readValue();
    std::string bat = batCharacteristic->readValue();

       Serial.print("temp: ");
       printInt32Value(temp);

       Serial.print("humidity: ");
       printInt32Value(humidity);

       Serial.print("pressure: ");
       printInt32Value(pressure);      

       Serial.print("battery: ");
       printInt16Value(bat);  

  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  delay(20000); // Delay a second between loops.
} // End of loop
