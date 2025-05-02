// PROJECT  :EEPROM Burner
// PURPOSE  :Burns data onto an EEPROM chip
// COURSE   :ICS4U-E
// AUTHOR   :G. Staznik
// DATE     :27/10/24
// MCU      :328P (Nano)
// STATUS   :Working

#define dataPIN 2       //Defines where the Data pin is on the Ardiuno
#define clockPIN 4      //Defines where the clock pin is on the Ardiuno
#define latchPIN 3      //Defines where the latch pin is on the Ardiuno
#define writeEnable 13  //Defines where the write Enable pins is on the Ardiuno

#define IO0 5   //Defines the pin associated with I/O 0
#define IO7 12  //Defines the pin associated with I/O8

/* INTEGER VALUE DETERMINES WHICH SLOT THE CODE IS PUT IN */
#define pageNumber 0                                                                                             //defines the page where the code is sent
int code[16]{0x82, 0x10, 0x21, 0x62, 0xA0,};  //code goes in here


void setup() {                      //sets data
  pinMode(dataPIN, OUTPUT);         //set the data pin to output
  pinMode(clockPIN, OUTPUT);        //set the clock pin to output
  pinMode(latchPIN, OUTPUT);        //set the latch pin to output
  pinMode(writeEnable, OUTPUT);     //set write enable to output
  digitalWrite(writeEnable, HIGH);  //set write enable high first- prevents accidental low
  Serial.begin(9600);               //start the serial monitor
}

void loop() {  //null (nothing to be looped)
}

uint8_t read(uint8_t address) {               //Reads what is in the EEPROM
  setPinsInput(IO0, IO7);                     //set the pins IO0-IO7 to input
  setAddress(address, true);                  //sets the address to what is desired, OE true
  uint8_t data = 0;                           //creates a temporary int to hold the information
  for (uint8_t pin = IO7; pin >= IO0; pin--)  //loops once for each pin
    data = (data << 1) + digitalRead(pin);    //read the pin, and put it into data
  return data;                                //return data for the read function
}

void write() {              //Writes the code[] into EEPROM
  setPinsOutput(IO0, IO7);  //set IO0-IO7 as outputs

  for (uint8_t lineNumber; lineNumber < 16; lineNumber++) {      //loop for each line of code
    setAddress(lineNumber, false);                               //set the address for each line of code, OE false
    for (uint8_t pin = IO0; pin <= IO7; pin++)                   //loop for each pin (IO0-IO7)
      digitalWrite(pin, code[lineNumber] & (1 << (pin - IO0)));  //set the data into that pin
    digitalWrite(writeEnable, LOW);                              //Pulse the write enable Low for 1 microsecond (1000 ns)
    delayMicroseconds(1);                                        //1000 ns delay
    digitalWrite(writeEnable, HIGH);                             //put back to high
    delay(500);                                                  //delay 5 ms for saftey
  }
}

void setPinsOutput(uint8_t lowestPin, uint8_t highestPin) {  //Sets all the pins to outputs
  for (uint8_t pin = lowestPin; pin <= highestPin; pin++)    //for the range of pins...
    pinMode(pin, OUTPUT);                                    //set them as output
}

void setPinsInput(uint8_t lowestPin, uint8_t highestPin) {  //Sets all the pins to inputs
  for (uint8_t pin = lowestPin; pin <= highestPin; pin++)   //for the range of pins...
    pinMode(pin, INPUT);                                    //set them as input
}

void setAddress(uint8_t lineNumber, bool outputEnable) {                                                    //Sets the address in the 595s
  digitalWrite(latchPIN, LOW);                                                                              //set the latch to low
  shiftOut(dataPIN, clockPIN, LSBFIRST, lineNumber + (pageNumber << 4));                                    //shove in the lowest 8 bits of data
  shiftOut(dataPIN, clockPIN, LSBFIRST, lineNumber + (pageNumber << 4) >> 8 | (outputEnable ? 0x0 : 0x8));  //shove in the higest 8 bits of data & OE
  digitalWrite(latchPIN, HIGH);                                                                             //set the latch high
}