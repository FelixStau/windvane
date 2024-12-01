const int hallInput = 14;
bool state = 0;

void ICACHE_RAM_ATTR isr() {
  state = 1;
}

void setup() {
  pinMode(hallInput, INPUT);
  attachInterrupt(digitalPinToInterrupt(hallInput), isr, RISING);
  Serial.begin(115200);
  Serial.println();
  Serial.println("Hall Sensor Test");
}

void loop() {
  if (state == 1)
  {
    Serial.println("Triggered");
    state = 0;
  } else
    Serial.println("...");
  delay(1000);
}