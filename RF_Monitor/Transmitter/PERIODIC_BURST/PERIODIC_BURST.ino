#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

constexpr int CC1101_SCK = 18;
constexpr int CC1101_MISO = 19;
constexpr int CC1101_MOSI = 23;
constexpr int CC1101_CSN = 5;
constexpr int CC1101_GDO0 = 4;
constexpr int CC1101_GDO2 = 27;

constexpr uint32_t STARTUP_QUIET_MS = 1000;
constexpr uint32_t BURST_ON_MS = 50;
constexpr uint32_t BURST_GAP_MS = 20;

CC1101 radio = new Module(
  CC1101_CSN,
  CC1101_GDO0,
  RADIOLIB_NC,
  CC1101_GDO2
);

void haltOnError(const char* operation, int16_t state) {
  if (state == RADIOLIB_ERR_NONE) {
    return;
  }

  Serial.printf("%s failed, error %d\n", operation, state);
  while (true) {
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  delay(STARTUP_QUIET_MS);

  SPI.begin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CSN);

  int16_t state = radio.begin(433.92, 4.8, 5.0, 135.0, 10, 16);
  haltOnError("CC1101 initialization", state);

  pinMode(CC1101_GDO2, OUTPUT);
  digitalWrite(CC1101_GDO2, LOW);
  Serial.println("PERIODIC_BURST active");
}

void loop() {
  int16_t state = radio.transmitDirect();
  haltOnError("Periodic burst start", state);
  delay(BURST_ON_MS);

  radio.standby();
  delay(BURST_GAP_MS);
}
