#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

constexpr int CC1101_SCK  = 18;
constexpr int CC1101_MISO = 19;
constexpr int CC1101_MOSI = 23;
constexpr int CC1101_CSN  = 5;
constexpr int CC1101_GDO0 = 4;
constexpr int CC1101_GDO2 = 27;

CC1101 radio = new Module(
  CC1101_CSN,
  CC1101_GDO0,
  RADIOLIB_NC,
  CC1101_GDO2
);

void setup() {
  Serial.begin(115200);
  delay(1000);

  SPI.begin(
    CC1101_SCK,
    CC1101_MISO,
    CC1101_MOSI,
    CC1101_CSN
  );

  int16_t state = radio.begin(
    433.92,
    4.8,
    5.0,
    135.0,
    10,
    16
  );

  Serial.printf("CC1101 initialization status: %d\n", state);
  if (state != RADIOLIB_ERR_NONE) {
    while (true) {
      delay(1000);
    }
  }

  // Hold the synchronous direct-mode data input at a known level.
  pinMode(CC1101_GDO2, OUTPUT);
  digitalWrite(CC1101_GDO2, LOW);

  state = radio.transmitDirect();
  Serial.printf("Continuous 433.92 MHz carrier status: %d\n", state);

  if (state != RADIOLIB_ERR_NONE) {
    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  // The CC1101 remains in direct transmit mode until reset or standby.
  delay(1000);
}
