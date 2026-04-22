#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

// Definiamo i pin sicuri per l'I2C sulla tua scheda CAM
#define I2C_SDA_PIN 18
#define I2C_SCL_PIN 17

float average_current_mA = 0;

void setup(void) {
  Serial.begin(115200);
  while (!Serial) {
      delay(1);
  }

  Serial.println("Inizializzazione I2C sui pin custom...");
  // Diciamo alla ESP32 di usare i pin 4 e 5 per parlare col sensore
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Initialize the INA219
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

  Serial.println("Measuring voltage and current with INA219 ...");
}

void loop(void) {
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);

  average_current_mA = (average_current_mA * 9 + current_mA) / 10;

  Serial.printf(">busvoltage:%.2f\n",busvoltage);
  //Serial.print("\t");
  Serial.printf(">shuntvoltage:%.2f\n",shuntvoltage);
  //Serial.print("\t");
  Serial.printf(">loadvoltage:%.2f\n",loadvoltage);
  //Serial.print("\t");
  Serial.printf(">current_mA:%.2f\n",current_mA);
  //Serial.print("\t");
  Serial.printf(">power_mW:%.2f\n",power_mW);

  Serial.printf(">average_current_mA:%.2f\n",average_current_mA);

  delay(50);
}