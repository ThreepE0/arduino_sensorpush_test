#include "esphome.h"
#include <BLEDevice.h>

#include "esphome/core/log.h"


static BLEUUID serviceUUID("EF090000-11D6-42BA-93B8-9DD7EC090AB0");
static BLEUUID macUUID("EF09000D-11D6-42BA-93B8-9DD7EC090AA9");
static BLEUUID tempUUID("EF090080-11D6-42BA-93B8-9DD7EC090AA9");
static BLEUUID batUUID("EF090007-11D6-42BA-93B8-9DD7EC090AA9");
static BLEUUID humUUID("EF090081-11D6-42BA-93B8-9DD7EC090AA9");
static BLEUUID barUUID("EF090082-11D6-42BA-93B8-9DD7EC090AA9");
static BLEUUID ledUUID("EF09000C-11D6-42BA-93B8-9DD7EC090AA9");
static BLEUUID rateUUID("EF090005-11D6-42BA-93B8-9DD7EC090AA9");

bool macStringToByteArray(const std::string& macStr, uint8_t* macArray) {
    if (macStr.length() != 17) return false;
    for (int i = 0; i < 6; i++) {
        uint8_t byteVal;
        if (sscanf(macStr.c_str() + i * 3, "%2hhx", &byteVal) != 1) return false;
        macArray[i] = byteVal;
    }
    return true;
}

std::string getMacAddress(const std::string& value) {
if (value.length() == 6) {
    char macAddress[18]; // Buffer for the MAC address string (including null-terminator).
    sprintf(macAddress, "%02X:%02X:%02X:%02X:%02X:%02X",
            value[5], value[4], value[3], value[2], value[1], value[0]);

    return std::string(macAddress);
} else {
    return "invalid mac";
}
}


class SensorPush : public PollingComponent {
 public:


  // MAC address of the target SensorPush device
  std::string mac_address;

  // Define sensors
  //esphome::text_sensor::TextSensor *mac_sensor = nullptr;
  TextSensor *battery_sensor = new TextSensor();
  TextSensor *temperature_sensor = new TextSensor();  
  TextSensor *humidity_sensor = new TextSensor();
  TextSensor *pressure_sensor = new TextSensor();
  TextSensor *led_sensor = new TextSensor();
  TextSensor *rate_sensor = new TextSensor();

  // Constructor that accepts the MAC address and polling interval
  SensorPush(const std::string& mac, uint32_t interval) : PollingComponent(interval), mac_address(mac) {}

  void setup() override {
    BLEDevice::init("");
    //mac_sensor = new esphome::text_sensor::TextSensor();
    // Initialization logic, e.g., BLE setup
    // This will run once when the ESP32 starts up
  }

  void update() override {
    // Create a new BLE client
    BLEClient* pClient  = BLEDevice::createClient();


    uint8_t macByteArray[6];
    if (!macStringToByteArray(mac_address, macByteArray)) {
        // Handle invalid MAC address format
        return;
    }
    // Connect to the remote BLE server
    if (!pClient->connect(macByteArray)) {
        // Handle connection failure
        return;
    }


    // Obtain a reference to the service and characteristics you want to interact with
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        pClient->disconnect();
        return;
    }

    //BLERemoteCharacteristic* pMacCharacteristic = pRemoteService->getCharacteristic(macUUID);
    BLERemoteCharacteristic* pBatCharacteristic = pRemoteService->getCharacteristic(batUUID);
    BLERemoteCharacteristic* pTempCharacteristic = pRemoteService->getCharacteristic(tempUUID);
    BLERemoteCharacteristic* pHumCharacteristic = pRemoteService->getCharacteristic(humUUID);
    BLERemoteCharacteristic* pBarCharacteristic = pRemoteService->getCharacteristic(barUUID);
    BLERemoteCharacteristic* pLedCharacteristic = pRemoteService->getCharacteristic(ledUUID);
    BLERemoteCharacteristic* pRateCharacteristic = pRemoteService->getCharacteristic(rateUUID);
    // ... repeat for other characteristics ...

    // Read the characteristics
    //std::string macValue = getMacAddress( pMacCharacteristic->readValue() );
    //std::string batValue = pBatCharacteristic->readValue();
    std::string rawTemp = pTempCharacteristic->readValue();
    int32_t tempIntValue = 0;
    if (rawTemp.size() >= sizeof(int32_t)) {
        memcpy(&tempIntValue, rawTemp.data(), sizeof(int32_t));
    }

    for (size_t i = 0; i < rawTemp.size(); i++) {
        ESP_LOGD("SensorPush", "Byte %d: 0x%02X", i, rawTemp[i]);
    }

    std::string tempValueStr = std::to_string(tempIntValue);

    //std::string humValue = pHumCharacteristic->readValue();
    //std::string barValue = pBarCharacteristic->readValue();
    //std::string ledValue = pLedCharacteristic->readValue();
    //std::string rateValue = pRateCharacteristic->readValue();

    // Convert the std::string values to their respective data types
    //int16_t tempValue, humidityValue, pressureValue, batValue, ledValue, rateValue;
    //memcpy(&tempValue, temp.data(), sizeof(int16_t));

    //memcpy(&humidityValue, humidity.data(), sizeof(int16_t));
    //memcpy(&pressureValue, pressure.data(), sizeof(int16_t));
    //memcpy(&batValue, bat.data(), sizeof(int16_t));
    //memcpy(&ledValue, led.data(), sizeof(int16_t));
    //memcpy(&rateValue, rate.data(), sizeof(int16_t));


    ESP_LOGD("SensorPush", "Temperature Value: %f", tempValueStr.c_str());  // Log the value
    // Convert and update ESPHome sensors
    //mac_sensor->publish_state(macValue);
    //battery_sensor->publish_state(std::stof(batValue));
    temperature_sensor->publish_state(rawTemp);
    //humidity_sensor->publish_state(std::stof(humValue));
    //pressure_sensor->publish_state(std::stof(barValue));
    //led_sensor->publish_state(std::stof(ledValue));
    //rate_sensor->publish_state(std::stof(rateValue));
    // ... repeat for other sensors ...

    // Disconnect from the BLE server
    pClient->disconnect();
  }
};
