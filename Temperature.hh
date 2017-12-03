#if TEMPERATURE == ENABLED
#if MODE == PREAMBLE

#include <DHT.h>

#elif MODE == DEFINITIONS

struct HumidityPinData {
  uint8_t pins[DIGITAL_PIN_BYTES];
};

struct HumidityData {
  uint8_t recording[16];
};

struct TemperatureData {
  int8_t recording[16];
};

struct HumidityPinData humidityPins;
struct HumidityData humidityData;
struct TemperatureData temperatureData;
DHT *sensors[16];

BLEService humidityService("E95D0600-251D-470A-A062-FA1922DFA9A7");
BLECharacteristic humidityPinChar("E95D0601-251D-470A-A062-FA1922DFA9A7", BLEWrite, sizeof(HumidityPinData));
BLECharacteristic humidityReadChar("E95D0602-251D-470A-A062-FA1922DFA9A7", BLERead | BLENotify, sizeof(HumidityData));
BLECharacteristic temperatureReadChar("E95D0603-251D-470A-A062-FA1922DFA9A7", BLERead | BLENotify, sizeof(TemperatureData));

void setupDHT();
void updateDHT();
void humidityPinWritten(BLECentral &central, BLECharacteristic &characteristic);

#elif MODE == FUNCTIONS

void setupDHT() {
#if !ONE_SERVICE
  Serial.println("Configuring Temperature & Humidity Service...");
  blePeripheral.addAttribute(humidityService);
#endif
  blePeripheral.addAttribute(humidityPinChar);
  blePeripheral.addAttribute(humidityReadChar);
  blePeripheral.addAttribute(temperatureReadChar);
  humidityPinChar.setEventHandler(BLEWritten, humidityPinWritten);
  for (int i = 0; i < 16; i++) {
    sensors[i] = NULL;
  }
}

void humidityPinWritten(BLECentral &central, BLECharacteristic &characteristic) {
  uint8_t newPins[DIGITAL_PIN_BYTES];
  uint8_t sensor = 0;
  memcpy((void *)&newPins, characteristic.value(), DIGITAL_PIN_BYTES);
  for (int i = 0; i < 2; i++) {
    for (uint8_t j = 0x80; j != 0; j >>= 1) {
      Serial.print("humidity pin[j] = ");
      Serial.println(humidityPins.pins[i] & j);
      Serial.print("new pin[j] = ");
      Serial.println(newPins[i] & j);
      if ((humidityPins.pins[i] & j) < (newPins[i] & j)) {
        // new DHT
        Serial.print("Creating new DHT on pin ");
        Serial.println(sensor);
        sensors[sensor] = new DHT(sensor, DHT22);
        sensors[sensor]->begin();
      } else if ((humidityPins.pins[i] & j) > (newPins[i] & j)) {
        // delete DHT
        Serial.print("Deleting DHT on pin ");
        Serial.println(sensor);
        delete sensors[sensor];
        sensors[sensor] = NULL;
      }
      sensor++;
    }
  }
  memcpy((void *)&humidityPins, newPins, sizeof(humidityPins));
}


void updateHumidityTemperatureData() {
  bool updated = false;
  for (int i = 0; i < 16; i++) {
    if (sensors[i] != NULL) {
      humidityData.recording[i] = (uint8_t) (sensors[i]->readHumidity());
      temperatureData.recording[i] = (int8_t) (sensors[i]->readTemperature());
      Serial.print("Humidity: ");
      Serial.print(humidityData.recording[i]);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.print(temperatureData.recording[i]);
      Serial.println(" *C");
      updated = true;
    }
  }
  if (updated) {
    Serial.println("Updating Temp/Humidity Characteristics");
    humidityReadChar.setValue(reinterpret_cast<byte *>(&humidityData), sizeof(humidityData));
    temperatureReadChar.setValue(reinterpret_cast<byte *>(&temperatureData), sizeof(temperatureData));
  }
}

#elif MODE == SETUP

setupDHT();

#elif MODE == UPDATE

updateHumidityTemperatureData();

#endif
#endif

