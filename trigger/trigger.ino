
const int pin_trigger = 12;//Pin to use for triggering
const unsigned int high_time_us = 150;//Time the signal spend to high, microseconds

void setup() 
{
  pinMode(pin_trigger, OUTPUT);
  Serial.begin(115200);
}

void send_trigger()
{
  digitalWrite(pin_trigger,HIGH);
  delayMicroseconds(high_time_us);
  digitalWrite(pin_trigger,LOW);
}

void loop() 
{
  if(Serial.available() > 0)
  {
    int incoming_byte = Serial.read();
    if(incoming_byte == 1)
    {
      send_trigger();
      Serial.write(0xFF);
    }
  }
  /*
  send_trigger();
  Serial.write(0xff);
  delay(63);*/

}
