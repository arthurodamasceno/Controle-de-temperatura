#include "max6675.h"

int thermoDO = 3;
int thermoCS = 8;
int thermoCLK = 9;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

uint8_t tempdata[16];

union teste {
  uint8_t b[4];
  float f;
};
union teste temp;

void setup() {
  pinMode(11, OUTPUT);
  Serial.begin(115200);

}

void loop() {
  analogWrite(11, 64);
  temp.f = thermocouple.readCelsius();

  Serial.write(0xAA);
  for (int i = 0; i < 4; i++) {
    Serial.write(temp.b[i]);
  }
  delay(200);
}
