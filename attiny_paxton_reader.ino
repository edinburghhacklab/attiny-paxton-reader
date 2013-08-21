/*

Pin 2 reader clock, PB3
Pin 3 reader data, PB4
Pin 5 I2C data
Pin 7 I2C clock, INT0

*/

#include <avr/io.h>
#include <stdint.h>            // has to be added to use uint8_t
#include <avr/interrupt.h>    // Needed to use interrupts
#include <TinyWireS.h>

volatile uint8_t portbhistory = 0xFF;     // default is high because the pull-up

#define DATA_MAX 10
#define READER_DATA_PIN 4
#define READER_CLOCK_PIN 3
#define LED_PIN 1

volatile int idx_byte = 0;
volatile int idx_bit = 0;
volatile int newdata = 0;
volatile unsigned long last = 0;
volatile unsigned long lastclock = 0;
volatile byte data[DATA_MAX] = {0};
volatile boolean led_state = 0;
volatile boolean readytosend = 0;

void setup()
{
  pinMode(READER_CLOCK_PIN, INPUT);
  pinMode(READER_DATA_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, led_state);
  
  GIMSK |= (1 << PCIE);     // set PCIE0 to enable PCMSK0 scan
  PCMSK |= (1 << PCINT3);   // set PCINT3 to trigger an interrupt on state change 

  sei();                     // turn on interrupts
  
  TinyWireS.begin(10);
}

void loop()
{
  // TODO: check timing on scope and determine optimum timeout
  // TODO: store time that data was recorded and only serve it over I2C if it is still fresh
  if (newdata && millis() - last > 10) {
    readytosend = 1;
    idx_byte = 0;
    idx_bit = 0;
    newdata = 0;
    led_state = !led_state;
    digitalWrite(LED_PIN, led_state);
  }
  if (TinyWireS.available() && readytosend) {
    for (int i = 0; i < DATA_MAX; i++) {
      TinyWireS.send(data[i]);
      data[i] = 0;
    }
    readytosend = 0;
  }
}

ISR (PCINT0_vect)
{
    uint8_t changedbits;

    changedbits = PINB ^ portbhistory;
    portbhistory = PINB;
 
    // TODO: check readytosend (or other) to avoid updating data during an i2c operation

    if(changedbits & (1 << PB3)) {
     if ((PINB & (1 << PB3)) == 0){
      if (micros() - lastclock > 400) {
        if (idx_byte >= DATA_MAX) return;
    
        byte data_bit = (PINB & (1<<PB4)) >> PB4;
        data[idx_byte] |= data_bit << (7-idx_bit);
        idx_bit++;
        if (idx_bit >= 8) {
          idx_byte++;
          idx_bit = 0;
        }
        last = millis();
        lastclock = micros();
        newdata = 1;
      }
     }
    }
}
