#include <SPI.h>
#include <107-Arduino-MCP2515.h>
/* ... */
static int const MKRCAN_MCP2515_CS_PIN  = 10;
static int const MKRCAN_MCP2515_INT_PIN = 9;
/* ... */
void onReceiveBufferFull(uint32_t const timestamp_us, uint32_t const id, uint8_t const * data, uint8_t const len)
{
  Serial.println(id, HEX);
}
void onTransmitBufferEmpty(ArduinoMCP2515 * this_ptr)
{
  /* You can use this callback to refill the transmit buffer via this_ptr->transmit(...) */
}
/* ... */
ArduinoMCP2515 mcp2515([](){ digitalWrite(MKRCAN_MCP2515_CS_PIN, LOW); },
                       [](){ digitalWrite(MKRCAN_MCP2515_CS_PIN, HIGH); },
                       [](uint8_t const d) -> uint8_t { return SPI.transfer(d); },
                       micros,
                       onReceiveBufferFull,
                       onTransmitBufferEmpty);

void IRAM_ATTR isr(void)
{
  Serial.println("Interrupt!");
  mcp2515.onExternalEventHandler();
}
/* ... */
void setup()
{
  Serial.begin(115200);
  while(!Serial) { }

  SPI.begin();
  pinMode(MKRCAN_MCP2515_CS_PIN, OUTPUT);
  digitalWrite(MKRCAN_MCP2515_CS_PIN, HIGH);

  pinMode(MKRCAN_MCP2515_INT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(MKRCAN_MCP2515_INT_PIN), isr, RISING);

  mcp2515.begin();
  mcp2515.setBitRate(CanBitRate::BR_250kBPS_16MHZ); // CAN bitrate and clock speed of MCP2515
  mcp2515.setNormalMode();
}

void loop()
{
  uint8_t const data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  Serial.println("Sent Message");
  mcp2515.transmit(1 /* id */, data, 8 /* len */);
  delay(100);
}