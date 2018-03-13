///////////////////////////////////////////////////////////////////////
// Available as stomp_delay.pde from openmusiclabs.com
// his program plays back a delayed sample. 
// the delay time is set by the up/dwn buttons
//
// Modified for use with ATMega1284P by Tobias van Dyk January 2018
///////////////////////////////////////////////////////////////////////

#define SIZE 7000 // circular buffer size  - the byte size is twice this
int buffer[SIZE]; // data buffer = SIZE * 2 bytes
unsigned int location = 0; // current buffer location
int data_buffer = 0x8000;  // to convert to and from signed int
byte ChkButton = 128;
int counter = SIZE/1;
byte EffectEnable = 1;  // use effects

#define LED PB0
#define FOOTSWITCH PB1
#define PUSHBUTTON_1 A5
#define PUSHBUTTON_2 A4

static inline void StompShield_init(void) 
{
  // setup ADC to sample on ADC0
  ADMUX = 0x60;  // left adjust, adc0, internal vcc as reference
  ADCSRA = 0xe5; // turn on adc, ck/32, auto trigger
  ADCSRB =0x06;  // timer1 overflow as trigger
  DIDR0 = 0x01;  // turn off digital inputs for adc0

  // setup PWM to output at 31.25ksps
  TCCR1A = 0xa0;  // set to compare mode to positive output
  TCCR1B = 0x11;  // ck/1, phase and frequency correct mode, ICR1 as top
  TIMSK1 = 0x01;  // turn on overflow interrupt

  // define pwm frequency - 31.3KHz
  ICR1H = 0x00;   // clear temporary high byte register
  ICR1L = 0xff;   // set top of counter

  DDRD |= 0x30;   // set pwm outputs (PD4 and PD5) to output PWM
  sei();          // enable interrupts
}

void setup() 
{
  //setup IO
  pinMode(FOOTSWITCH, INPUT_PULLUP);
  pinMode(PUSHBUTTON_1, INPUT_PULLUP);
  pinMode(PUSHBUTTON_2, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  StompShield_init(); // setup the arduino for the shield

  digitalWrite(LED, HIGH); 
}

void loop() 
{
  // Turn on the LED if the effect is ON.
  if (ChkButton == 129) 
  {
  if (!digitalRead(FOOTSWITCH)) 
     { if (EffectEnable == 1) 
          { digitalWrite(LED, LOW); 
            EffectEnable = 0;
            counter = 1;
          } else { digitalWrite(LED, HIGH); 
                   EffectEnable = 1;
                   counter = SIZE;
                 }
     }
  }
                          
}

ISR(TIMER1_OVF_vect) {
  // output the last value calculated
  OCR1AL = ((data_buffer + 0x8000) >> 8); // convert to unsigned, send out high byte
  OCR1BL = data_buffer;                   // send out low byte
    
  // get ADC data
  byte temp1 = ADCL; // you need to fetch the low byte first
  byte temp2 = ADCH; // yes it needs to be done this way
  int input = ((temp2 << 8) | temp1) + 0x8000; // make a signed 16b value
  

  ChkButton--; // check buttons every 256 times
  if (ChkButton == 0) 
     { if (!digitalRead(PUSHBUTTON_1))  // increase the delay
          { if (counter < SIZE) counter = counter + 1;
            else counter = SIZE;  
            //digitalWrite(LED, LOW);   // unblinks the led
          }

      if (!digitalRead(PUSHBUTTON_2))  // decrease the delay
         { if (counter > 1) counter = counter - 1;
           else counter = 1;
           //digitalWrite(LED, LOW);    // unblinks the led
         }
      }

  data_buffer = buffer[location]; // fetch current sample
  
  buffer[location] = input;                                  // store new sample
  // buffer[location] = (input + buffer[location])>>1;       // Use this for echo effct
  
  location++;                             // increment to next location
  if (location >= counter) location = 0;  // boundary wrap

}


