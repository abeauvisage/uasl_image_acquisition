
const int pin_trigger = 9;//Pin to use for triggering
const unsigned int high_time_us = 150;//Time the signal spend to high, microseconds

void setup() 
{
  pinMode(pin_trigger, OUTPUT);
  digitalWrite(pin_trigger,HIGH);
  Serial.begin(115200);
}

void send_trigger()
{
  digitalWrite(pin_trigger,LOW);
  delayMicroseconds(high_time_us);
  digitalWrite(pin_trigger,HIGH);
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

/*  send_trigger();
  Serial.write(0xff);
  delay(63);*/

}
