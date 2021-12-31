#include <math.h>

#define ADDR_CLOCK 0

#define ENCODER_CLOCK 2
#define ENCODER_DATA 1

#define ENCODER_SWITCH 3

#define TRIGGER 4

#define VALUE_1 5
#define VALUE_8 12

#define DISPLAY_TENS 15
#define DISPLAY_ONES 16

#define SHIFT_CLOCK 17
#define SHIFT_DATA 18
#define SHIFT_STROBE 19

#define NOP 0x00
#define LDA 0x10
#define ADD 0x20
#define SUB 0x30
#define STA 0x40
#define LDI 0x50
#define JMP 0x60
#define JIC 0x70
#define JWZ 0x80
#define OUT 0xE0
#define HLT 0xF0

#define DEBOUNCE 10

//////////////////////////
/* SEVEN SEGMENT DIGITS */
//////////////////////////

byte seven[] = { 
  0x3F, 0x0C,
  0xAB, 0xAD,
  0x9C, 0xB5,
  0xB7, 0x2C,
  0xBF, 0xBC,
};

byte te[] = { 0x2C, 0xB3, };
byte lp[] = { 0x13, 0xBA, };
byte fi[] = { 0xB2, 0x0C, };


//////////////
/* PROGRAMS */
//////////////

// test 
byte test[]= {
  0x00, 0x01, 0x02, 0x04,
  0x08, 0x10, 0x20, 0x40,
  0x80, 0xFF, 0x7F, 0x3F,
  0x1F, 0x0F, 0x07, 0x03,
};

// loop 0 - 254
byte looper[] = {
  LDA | 15,
  OUT | 0,
  ADD | 14,
  JIC | 6,
  STA | 15,
  JMP | 0,
  LDA | 15,
  OUT | 0,
  SUB | 14,
  JWZ | 0,
  STA | 15,
  JMP | 6,
  NOP | 0,
  NOP | 0,
  1,
  0,
};

// Fibonacci on repeat
byte fibonacci[] = { 
  LDI | 0x01,   //0
  STA | 0x0F,   //1
  LDI | 0x00,   //2
  STA | 0x0E,   //3
  OUT | 0x00,   //4
  ADD | 0x0F,   //5
  JIC | 0x00,   //6
  OUT | 0x00,   //7
  STA | 0x0F,   //8
  ADD | 0x0E,   //9
  JIC | 0x00,   //10
  OUT | 0x00,   //11
  JMP | 0x03,   //12
  NOP | 0x00,   //13
        0x00,   //14
        0x01,   //15
};

volatile byte* program = test;

volatile byte programTens = te[0];
volatile byte programOnes = te[1];

volatile int encoderPos = 0;
volatile unsigned long lastPress = 0;
volatile unsigned long lastTurn = 0;

void setup() {
  pinMode(ADDR_CLOCK, OUTPUT);
  digitalWrite(ADDR_CLOCK, HIGH);

  pinMode(TRIGGER, OUTPUT);  
  digitalWrite(TRIGGER, HIGH);


  pinMode(ENCODER_DATA, INPUT_PULLUP);

  pinMode(ENCODER_CLOCK, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLOCK), readEncoder, FALLING);

  pinMode(ENCODER_SWITCH, INPUT);
  digitalWrite(ENCODER_SWITCH, LOW);
  attachInterrupt(digitalPinToInterrupt(ENCODER_SWITCH), programComputer, FALLING); 

      
  pinMode(SHIFT_STROBE, OUTPUT);
  digitalWrite(SHIFT_STROBE, LOW);

  pinMode(SHIFT_CLOCK, OUTPUT);
  digitalWrite(SHIFT_CLOCK, LOW);

  pinMode(SHIFT_DATA, OUTPUT); 
  digitalWrite(SHIFT_DATA, LOW);


  pinMode(DISPLAY_TENS, OUTPUT);
  digitalWrite(DISPLAY_TENS, HIGH);

  pinMode(DISPLAY_ONES, OUTPUT);
  digitalWrite(DISPLAY_ONES, HIGH);
}

void readEncoder() {
  /* if both pins are the same, its spinning forwards
     if they are different, its spinning backwards 
  */
  int clk = digitalRead(ENCODER_CLOCK);
  int data = digitalRead(ENCODER_DATA);
  unsigned long ms = millis();
  if (ms - lastTurn > DEBOUNCE && ms - lastPress > DEBOUNCE) {  
    if (clk == data) {
      encoderPos < 2 ? encoderPos++ : encoderPos = 0;
    } else {
      encoderPos > 0 ? encoderPos-- : encoderPos = 2;
    }
    setProgram();
    setDisplay();
  }
  lastTurn = ms;
}

void setProgram() {
  switch(encoderPos) {
    case 0:
      program = test;
      break;
    case 1:
      program = looper;
      break;
    case 2:
      program = fibonacci;
      break;
  }
}

void setDisplay() {
  switch(encoderPos) {
    case 0:
      programTens = te[0];
      programOnes = te[1];
      break;
    case 1:
      programTens = lp[0];
      programOnes = lp[1];
      break;
    case 2:
      programTens = fi[0];
      programOnes = fi[1];
      break;
  }
}

void writeOnes(byte digit) {
  shiftOut(SHIFT_DATA, SHIFT_CLOCK, LSBFIRST, digit);
  
  digitalWrite(DISPLAY_TENS, HIGH);
  digitalWrite(SHIFT_STROBE, HIGH);
  
  digitalWrite(SHIFT_STROBE, LOW);
  digitalWrite(DISPLAY_ONES, LOW);
}

void writeTens(byte digit){
  shiftOut(SHIFT_DATA, SHIFT_CLOCK, LSBFIRST, digit);
  
  digitalWrite(DISPLAY_ONES, HIGH);
  digitalWrite(SHIFT_STROBE, HIGH);
  
  digitalWrite(SHIFT_STROBE, LOW);    
  digitalWrite(DISPLAY_TENS, LOW);
}

void writeSevenSegmentWithProgram(){
  writeOnes(programOnes);
  writeTens(programTens);
}

void writeSevenSegmentWithAddress(int address) {
  int ones;
  int tens;
  if (address >= 10) {
    ones = address - 10;
    tens = 1;
  }
  else {
    ones = address;
    tens = 0;
  }

  for (int i = 0; i < 500; i += 1) {
    writeOnes(seven[ones]);
    writeTens(seven[tens]);
  }
  
  digitalWrite(DISPLAY_ONES, HIGH);
  digitalWrite(DISPLAY_TENS, HIGH);
}

void incrementAddress() {
    digitalWrite(ADDR_CLOCK, LOW);
    delay(140);
    digitalWrite(ADDR_CLOCK, HIGH);
}


void setValue(byte value) {
  for (int pin = VALUE_8; pin >= VALUE_1; pin -= 1) {
    pinMode(pin, OUTPUT);
  }

  for (int pin = VALUE_8; pin >= VALUE_1; pin -= 1) {
      digitalWrite(pin, value & 1);
    value = value >> 1;
  }
}

void toggleTrigger() {
  digitalWrite(TRIGGER, LOW);
  delay(100);
  digitalWrite(TRIGGER, HIGH);
}

void programComputer() {
  unsigned long ms = millis();
  if (ms - lastPress > DEBOUNCE) {  
    for (int i = 0; i < 16; i+=1) {
      setValue(program[i]);
      delayMicroseconds(10000);
      toggleTrigger();
      writeSevenSegmentWithAddress(i);
      if (i != 15){
        incrementAddress();
      }
    }
  } 
  lastPress = ms;
}

void loop(){
  writeSevenSegmentWithProgram();
}
