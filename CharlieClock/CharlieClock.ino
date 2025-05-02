// PROJECT  :CharlieClock
// PURPOSE  :To demonstrate Charlieplexing
// COURSE   :ICS4U-E
// AUTHOR   :G. Staznik
// DATE     :24/9/24
// MCU      :328P (Nano)
// STATUS   :Working
#include <Wire.h>  // Includes the wire library for I2C
#define p1 A0      //Defines the pins for the Clock
#define p2 13
#define p3 12
#define p4 11
#define p5 10
#define p6 9
#define p7 8
#define p8 7
#define p9 6
#define p10 5
#define p11 1
#define p12 0
#define SELPIN 2  //Define the select pin
#define ADDPIN 3  //Define the add pin

#define RTCADDRESS 0x68  // Defines the I2C address of the RTC
#define SCL A5           //  Defines LATCH Pin
#define SDA A4           //  Defines CLOCK Pin

#define entriesBeforeSecs 0     //Number of entries before secs addresses in the pins[]
#define entriesBeforeMins 60    //Number of entries before the minutes addresses in pins[]
#define entriesBeforeHours 120  //Number of entries before the hours addresses in pins[]
#define numHours 12             //Number of hours (12 since the clock is 12 hour)
#define numMinAndSec 60         //Number of minutes/seconds

#define SECONDS 2  //Defines the index of the address of the seconds in TIME[]
#define MINUTES 1  //Defines the index of the address of the minutes in TIME[]
#define HOURS 0    //Defines the index of the address of the Hours in TIME[]


volatile uint8_t select = 0;  //Create an int to be changed when the select button is pressed
volatile uint8_t add = 0;     //Create an in to be changed when the add button is pressed

struct LED {        //Create the struct for the LEDs addresses (cathode, anode)
  uint8_t cathode;  //Create an int for the cathode
  uint8_t anode;    //Create an into for the anode
};

LED pins[]{ //Create the struct which holds all LEDs addresses in order
//Addresses for Seconds
{ p5, p1 }, { p5, p2 }, { p5, p3 }, { p5, p4 }, { p5, p6 }, { p5, p7 }, { p5, p8 }, { p5, p9 }, { p5, p10 }, { p5, p11 }, { p5, p12 }, { p4, p8 },
{ p4, p9 }, { p4, p10 }, { p4, p11 }, { p4, p12 }, { p9, p12 }, { p9, p11 }, { p9, p10 }, { p9, p8 }, { p9, p7 }, { p9, p6 }, { p9, p5 }, { p9, p4 }, { p9, p3 },
{ p9, p2 }, { p9, p1 }, { p8, p12 }, { p8, p11 }, { p8, p10 }, { p8, p9 }, { p8, p7 }, { p8, p6 }, { p8, p5 }, { p8, p4 }, { p8, p3 }, { p8, p2 }, { p8, p1 }, { p7, p12 },
{ p7, p11 }, { p7, p10 }, { p7, p9 }, { p7, p8 }, { p7, p6 }, { p7, p5 }, { p7, p4 }, { p7, p3 }, { p7, p2 }, { p7, p1 }, { p6, p1 }, { p6, p2 }, { p6, p3 }, { p6, p4 }, { p6, p5 },
{ p6, p7 }, { p6, p8 }, { p6, p9 }, { p6, p10 }, { p6, p11 }, { p6, p12 }, 
//Addresses for Minutes
{ p2, p8 }, { p2, p9 }, { p2, p10 }, { p2, p11 }, { p2, p12 }, { p12, p1 }, { p12, p2 }, { p12, p3 }, { p12, p4 }, { p12, p5 }, { p12, p6 }, { p12, p7 }, { p12, p8 }, { p12, p9 },
{ p12, p10 }, { p12, p11 }, { p11, p10 }, { p11, p9 }, { p11, p8 }, { p11, p7 }, { p11, p6 }, { p11, p5 }, { p11, p4 }, { p11, p3 }, { p11, p2 }, { p11, p1 }, { p10, p12 }, { p10, p11 },
{ p10, p9 }, { p10, p8 }, { p10, p7 }, { p10, p6 }, { p10, p5 }, { p10, p4 }, { p10, p3 }, { p10, p2 }, { p10, p1 }, { p4, p7 }, { p4, p6 }, { p4, p5 }, { p4, p3 }, { p4, p2 }, { p4, p1 },
{ p3, p1 }, { p3, p2 }, { p3, p4 }, { p3, p5 }, { p3, p6 }, { p3, p7 }, { p3, p8 }, { p3, p9 }, { p3, p10 }, { p3, p11 }, { p3, p12 }, { p2, p1 }, { p2, p3 }, { p2, p4 }, { p2, p5 }, { p2, p6 }, { p2, p7 }, 
//Addresses for Hours   
{ p1, p9 }, { p1, p10 }, { p1, p11 }, { p1, p12 }, { p11, p12 }, { p1, p5 }, { p1, p4 }, { p1, p3 }, { p1, p2 }, { p1, p6 }, { p1, p7 }, { p1, p8 } };

uint8_t TIME[]{ 0, 0, 0 };  //Create the array to hold the time's LED addresses

void setup() {                                                  //Initialize the code
  attachInterrupt(digitalPinToInterrupt(SELPIN), SEL, RISING);  //Create an interrupt for the select button
  attachInterrupt(digitalPinToInterrupt(ADDPIN), ADD, RISING);  //Create an interrupt for the add button
  Wire.begin();                                                 //Start the TinyWireM library
}

void loop() {                      //the looped code
  getTime();                       //Get the time
  for (uint8_t i = 0; i < 3; i++)  //loop 3 times for (secs, minutes, hours)
    display(i);                    // Display the time
  if (select)                      //if select has been pressed...
    timeChange();                  //allow the user to change the time
}

void SEL() {  //ran if the select button is pressed
  select++;   //add 1 to select
}

void ADD() {  //ran if the add button is pressed
  add++;      //add 1 to add
}

void timeChange() {                             //Allows the user to manually change the time
  while (select <= 3) {                         //limits to while select has been pressed less than 3 times
    getTime();                                  //Gets the time
    display(select - 1);                        //displays the time but only for the unit being changed
    if (add) {                                  //if the add button has been pressed...
      TIME[select - 1] = TIME[select - 1] + 1;  //Add one to the index of TIME[] for the unit being changed
      setTime();                                //Set the new time in the RTC
      add = 0;                                  //reset add for use again
    }
  }
  select = 0;  //reset select for reuse
}

void display(uint8_t pass) {                    //Charlieplexes the time on the clock
  pinMode(pins[TIME[pass]].anode, OUTPUT);      //Changes the anode of the LED to output
  pinMode(pins[TIME[pass]].cathode, OUTPUT);    //Changes the cathode of the LED to output
  digitalWrite(pins[TIME[pass]].anode, HIGH);   //Set the anode to HIGH
  digitalWrite(pins[TIME[pass]].cathode, LOW);  // Set the cathode to low
  delay(1);                                     //wait 1 ms (for POV)
  pinMode(pins[TIME[pass]].anode, INPUT);       //Change the anode back to high-Z
  pinMode(pins[TIME[pass]].cathode, INPUT);     //Change the cathode back to high-Z
}

void getTime() {                       // Fetches the time from the RTC
  Wire.beginTransmission(RTCADDRESS);  // Begin an I2C communication at the RTC address
  Wire.write(0);                       // Start reading from Register 0
  Wire.endTransmission();              // End the Transmission
  Wire.requestFrom(RTCADDRESS, 3);     // Communicate what data is requested
  while (!Wire.available())
    ;                                                                  // Wait for the address to be ready
  TIME[SECONDS] = BCD2DEC(Wire.read()) + entriesBeforeSecs;            //Convert the RTC data and add the number of entries before seconds in the pins[]
  TIME[MINUTES] = BCD2DEC(Wire.read()) + entriesBeforeMins;            //Convert the RTC data and add the number of entries before minutes in the pins[]
  TIME[HOURS] = BCD2DEC(Wire.read()) % numHours + entriesBeforeHours;  //Convert the RTC data and add the number of entries before minutes in the pins[] (divide by number of hours)
}

void setTime() {                                                            // Sets the Time in the RTC
  Wire.beginTransmission(RTCADDRESS);                                       // Begin an I2C communication at the RTC address
  Wire.write(0);                                                            // Start writing at Register 0 (seconds)
  Wire.write(DEC2BCD((TIME[SECONDS]) % numMinAndSec));                      //Send the seconds, (divide the pins's address index by 60 to get seconds)
  Wire.write(DEC2BCD((TIME[MINUTES] % entriesBeforeMins) % numMinAndSec));  //Send the minutes, (divide by the entries before mins in pins[] and number of minutes)
  Wire.write(DEC2BCD((TIME[HOURS] % entriesBeforeHours) % numHours));       //Send the hours, (divide by the entries before hours in pins[] and number of hours)
  Wire.endTransmission();                                                   // End the transmissions
}

uint8_t DEC2BCD(uint8_t value) {            // One of the Utility Conversion Functions
  return (value / 10 << 4) + (value % 10);  //Converts DEC # to BCD
}

uint8_t BCD2DEC(uint8_t value) {              // The other Utility Conversion Functions
  return (value >> 4) * 10 + (value & 0x0F);  //Converts BCD # to DEC
}