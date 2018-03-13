//////////////////////////////////////////////////////////////////////
// stomp_flanger.pde from openmusiclabs.com 
// this program does a flanger effect, and interpolates between samples
// for a smoother sound output.  a rampwave is used to set the variable
// delay.  the min and max delay times it swing between is set by MIN
// and MAX.  these are in samples, divide by 31.25ksps to get ms delay
// times.  the two up/dwn buttons determines how much the ramp increments
// by each time.  this sets the frequency of the delay sweep, which is
// min/maxed by B_MIN/B_MAX.
//
// Modified for use with ATMega1284P by Tobias van Dyk January 2018
///////////////////////////////////////////////////////////////////////

#define LED PB0
#define FOOTSWITCH PB1
#define PUSHBUTTON_1 A5
#define PUSHBUTTON_2 A4

#define MIN 10        // try 1 -100, 10 = min delay of ~300us
#define MAX 200       // try 1000 - 200, 200 = max delay of ~6ms
#define SIZE MAX + 10 // data buffer size - must be more than MAX + MIN
#define B_MAX 10      // max value (fast sweep freq)
#define B_MIN 1       // min value (slow sweep freq)

int buffer[SIZE]; // create a data buffer
byte dir = 1; // keeps track of up/down counting
unsigned int location = 0; // incoming data buffer pointer
int ChkButton = 512 ; // button checking timer
byte counter = B_MIN; // counter (and start value)
unsigned int fractional = 0x00; // fractional sample position
int data_buffer; // temporary data storage to give a 1 sample buffer
byte EffectEnable = 1;  // use effects

/////////////////////////////////////////////////////////////////////////////////////
// openmusiclabs 6.8.11
// taken from http://mekonik.wordpress.com/2009/03/18/arduino-avr-gcc-multiplication/
/////////////////////////////////////////////////////////////////////////////////////
// multiplies 16 bit number X 8 bit and stores 2 high bytes
#define MultiSU16X8toH16(intRes, int16In, int8In) \
asm volatile ( \
"clr r26 \n\t"\
"mulsu %B1, %A2 \n\t"\
"movw %A0, r0 \n\t"\
"mul %A1, %A2 \n\t"\
"add %A0, r1 \n\t"\
"adc %B0, r26 \n\t"\
"clr r1 \n\t"\
: \
"=&r" (intRes) \
: \
"a" (int16In), \
"a" (int8In) \
:\
"r26"\
)


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
  if (!counter) counter = 0 ;
                     
}

ISR(TIMER1_OVF_vect) { // all processing happens here

  // output the last value calculated
  OCR1AL = ((data_buffer + 0x8000) >> 8); // convert to unsigned, send out high byte
  OCR1BL = data_buffer; // send out low byte
  
  // get ADC data
  byte temp1 = ADCL; // you need to fetch the low byte first
  byte temp2 = ADCH; // yes it needs to be done this way
  int input = ((temp2 << 8) | temp1) + 0x8000; // make a signed 16b value
  
  ChkButton++; //if (ChkButton < 1023) ChkButton++; else ChkButton--;// check buttons every 256*256 times
  if (ChkButton == 128) 
     { if (!digitalRead(PUSHBUTTON_1))   
          { if (counter > B_MIN) counter = counter - 1; // if not at min, decrement
          }

      if (!digitalRead(PUSHBUTTON_2))   
         { if (counter < B_MAX) counter++; // if not at max, increment
         }
      }
 
  // fetch/store data
  buffer[location] = input; // store current sample
  location++; // go to next sample position
  if (location >= SIZE) location = 0; // deal with buffer wrap
  
  int temp = location - (fractional >> 8); // find delayed sample
  if (temp < 0) temp += SIZE; // deal with buffer wrap
  int output = buffer[temp]; // fetch delayed sample
  temp -= 1; // find adjacent sample
  if (temp < 0) temp += SIZE; // deal with buffer wrap
  int output2 = buffer[temp]; // get adjacent sample
  
  // interpolate between adjacent samples
  int temp4; // create some temp variables
  int temp5;
  // multiply by distance to fractional position
  MultiSU16X8toH16(temp4, output, (0xff - (fractional & 0x00ff)));
  MultiSU16X8toH16(temp5, output2, (fractional & 0x00ff));
  output = temp4 + temp5; // sum weighted samples
  // save value for playback next interrupt
  data_buffer = output; 
  
  // up or down count as necessary till MIN/MAX is reached
  if (dir) {
    if ((fractional >> 8) >= MAX) dir = 0;
    fractional += counter;
    //digitalWrite(LED, LOW);
  }
  else {
    if ((fractional >> 8) <= MIN) dir = 1;
    fractional -= counter;
    //digitalWrite(LED, HIGH);
  } 
  
}


