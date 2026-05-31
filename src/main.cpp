#include <SPI.h>
#include <ACAN2517FD.h>

const byte MCP2518FD_CS  = 5;
const byte MCP2518FD_INT = 4;

ACAN2517FD can(MCP2518FD_CS, SPI, MCP2518FD_INT);

static void IRAM_ATTR canISR() { can.isr(); }

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Serial.println("Initializing MCP2518FD...");

  SPI.begin(18, 19, 23, MCP2518FD_CS);

  ACAN2517FDSettings settings(ACAN2517FDSettings::OSC_20MHz, 500UL * 1000UL, ACAN2517FDSettings::DATA_BITRATE_x8);

  // 75% sample point matches Arduino Due / GVRET due_can default at 500 kbps.
  // 20 MHz / 500 kbps = 40 TQ/bit; sample at TQ 30 = 75%.
  settings.mBitRatePrescaler        = 1;
  settings.mArbitrationPhaseSegment1 = 29;
  settings.mArbitrationPhaseSegment2 = 10;
  settings.mArbitrationSJW           = 4;
  settings.mDataSJW                  = 1;

  uint32_t errorCode = can.begin(settings, canISR);

  if (errorCode == 0) {
    Serial.println("MCP2518FD initialized successfully!");
  } else {
    Serial.print("Initialization failed: 0x");
    Serial.println(errorCode, HEX);
    while (1) { delay(1000); }
  }
}

void loop() {
  // --- Receive ---
  CANFDMessage rxMessage;
  if (can.receive(rxMessage)) {
    Serial.print("RX ID: 0x");
    Serial.print(rxMessage.id, HEX);
    Serial.print("  len:");
    Serial.print(rxMessage.len);
    Serial.print("  data:");
    for (int i = 0; i < rxMessage.len; i++) {
      Serial.print(" ");
      Serial.print(rxMessage.data[i], HEX);
    }
    Serial.println();
  }

  // --- Transmit every 2 seconds ---
  static uint32_t lastTxTime = 0;
  if (millis() - lastTxTime > 2000) {
    lastTxTime = millis();

    CANFDMessage txMessage;
    txMessage.id      = 0x1C1;
    txMessage.type    = CANFDMessage::CAN_DATA;
    txMessage.ext     = false;
    txMessage.len     = 8;
    txMessage.data[0] = 0xAA;
    txMessage.data[1] = 0xBB;
    txMessage.data[2] = 0xCC;
    txMessage.data[3] = 0xDD;
    txMessage.data[4] = 0xEE;
    txMessage.data[5] = 0xFF;
    txMessage.data[6] = 0x11;
    txMessage.data[7] = 0x22;

    if (!can.tryToSend(txMessage)) {
      Serial.println("TX failed");
    }
  }
}
