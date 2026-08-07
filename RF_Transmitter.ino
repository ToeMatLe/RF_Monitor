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

uint32_t packetNumber = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  SPI.begin(
    CC1101_SCK,
    CC1101_MISO,
    CC1101_MOSI,
    CC1101_CSN
  );

  Serial.print("Initializing CC1101... ");

  int16_t state = radio.begin(
    433.92,  // Frequency in MHz
    4.8,     // Bit rate in kbps
    5.0,     // Frequency deviation in kHz
    135.0,   // Receiver bandwidth in kHz
    10,      // Output power in dBm
    16       // Preamble length in bits
  );

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("failed, error ");
    Serial.println(state);

    while (true) {
      delay(1000);
    }
  }

  Serial.println("success!");
}

void loop() {
  Serial.println("Carrier ON");

  int16_t state = radio.transmitDirect();

  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("Direct TX failed: %d\n", state);
  }

  delay(5000);

  radio.standby();
  Serial.println("Carrier OFF");

  delay(5000);
}