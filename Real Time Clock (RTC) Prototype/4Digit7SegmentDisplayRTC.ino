// PROJECT  :4Digit7SegmentDisplay1234
// PURPOSE  :Set and display time
// COURSE   :ICS3U
// AUTHOR   :Goran Staznik
// DATE     :2024 02 20
// MCU      :85
// STATUS   :Working
// REFERENCE: https://github.com/adafruit/TinyWireM

#include <TinyWireM.h>  //Include libraries & board manager
#include <USI_TWI_Master.h>

#define RTCADDRESS 0x68  //I2C address for DS1307

#define DATA 0      //  SDA:0, SCL:4
#define LATCH 3     //
#define CLOCK 4     //
#define INTERVAL 4  // former delay duration
#define TIMEON 1    //  The TIMEON/TIMEOFF ratio changes the duty cycle
#define TIMEOFF 1   //  of the display, hence the brightness.

#define SQWREGISTER 7  //Local address in DS1307 for SQW options
#define SQWE 4
#define RS0 0
#define RS1 1
#define HZ1 0 << RS1 | 0 << RS0  //1   Hz


uint8_t buffer[] = { 0, 0, 0, 0 };  //intermediate data holding array
uint8_t numbers[] = {
  //7-segment digit map
  //B0abcdefg
  B01111110,  //0
  B00110000,  //1
  B01101101,  //2
  B01111001,  //3
  B00110011,  //4
  B01011011,  //5
  B00011111,  //6
  B01110000,  //7
  B01111111,  //8
  B01110011   //9
};

void setup() {
  pinMode(DATA, OUTPUT);
  pinMode(CLOCK, OUTPUT);
  pinMode(LATCH, OUTPUT);

  setSQW(HZ1);  //set SQW to 1Hz
  setTime();    //Get current time and upload it
}

void loop() {
  getTime();   //get time from DS1307
  showTime();  //display the time on 7seg display
}


void setTime() {
  //parse the int from char (ASCII)
  uint8_t SETHOURS = ((__TIME__[0] & 0xF) << 4) + (__TIME__[1] & 0xF);
  uint8_t SETMINUTES = ((__TIME__[3] & 0xF) << 4) + (__TIME__[4] & 0xF);
  uint8_t SETSECONDS = ((__TIME__[6] & 0xF) << 4) + (__TIME__[7] & 0xF);

  TinyWireM.beginTransmission(RTCADDRESS);  //transmit the time to DS1307
  TinyWireM.write(0);
  TinyWireM.write(SETSECONDS);
  TinyWireM.write(SETMINUTES);
  TinyWireM.write(SETHOURS);
  TinyWireM.endTransmission();
}

void getTime() {
  TinyWireM.beginTransmission(RTCADDRESS);  //recive the time from DS1307
  TinyWireM.write(1);
  TinyWireM.endTransmission();
  TinyWireM.requestFrom(RTCADDRESS, 2);
  while (!TinyWireM.available())
    ;
  uint8_t minutes = BCD2DEC(TinyWireM.read());
  uint8_t hours = BCD2DEC(TinyWireM.read());

  buffer[3] = numbers[hours / 10];  //split the data into the digits & set the number
  buffer[2] = numbers[hours % 10];  //
  buffer[1] = numbers[minutes / 10];
  buffer[0] = numbers[minutes % 10];
}

void showTime() {  //Display the time
  for (uint8_t digit = 0; digit < 4; digit++) {
    digitalWrite(LATCH, LOW);
    shiftOut(DATA, CLOCK, LSBFIRST, ~(1 << digit));   //turn the digit on
    shiftOut(DATA, CLOCK, LSBFIRST, ~buffer[digit]);  //send the digit the LEDs to be on
    digitalWrite(LATCH, HIGH);
    delay(TIMEON);  //Delay for POV
    digitalWrite(LATCH, LOW);
    shiftOut(DATA, CLOCK, LSBFIRST, 0x0F);  //Clear
    shiftOut(DATA, CLOCK, LSBFIRST, 0xFF);
    digitalWrite(LATCH, HIGH);
    delay(TIMEOFF);
  }
}

void setSQW(uint8_t rate) {  //Set the SQW (for colon)
  TinyWireM.begin();
  TinyWireM.beginTransmission(RTCADDRESS);  //Set the speed
  TinyWireM.write(SQWREGISTER);
  TinyWireM.write(1 << SQWE | rate);
  TinyWireM.endTransmission();
}

uint8_t DEC2BCD(uint8_t value) {  //convert to BCD
  return (value / 10 << 4) + (value % 10);
}

uint8_t BCD2DEC(uint8_t value) {  //convert from BCD
  return (value >> 4) * 10 + (value & 0x0F);
}
