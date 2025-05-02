// PROJECT  : Automatic Tuner
// PURPOSE  : Finds frequency of note using arduinoFFT library
// AUTHOR   : Goran Staznik
// COURSE   : ICS3U-E
// DATE     : Dec 9, 2023
// MCU      : 328P (Nano)
// STATUS   : Working
// REFERENCE: https://github.com/kosme/arduinoFFT
// NOTES    :

#include <arduinoFFT.h>  //Include the FFT library

#define E 41.20  //Define note frequency constants (A=440)
#define A 55.00
#define D 73.42
#define G 98.00

#define letterE 0xF2  //Data for displaying each letter (first digit is discarded) E
#define letterA 0xEE  //A
#define letterD 0xBC  //D
#define letterG 0x7A  //G

#define SAMPLES 128     //Number of samples (data points) taken for a transform (MUST BE POWER OF 2)
#define SAMPLEFREQ 220  //Frequency of the samples taken (highest detecable pitch is freq/2 in hz)
#define LOWEND 35       //The lowest pitch that the FFT will return in hz (removes noise)
#define AUDIO_PIN A1    //Declare the input pin

#define BARDATA 2  //Declare shift register pins for Bargraph
#define BARCLOCK 4
#define BARLATCH 3
#define SEGDATA 6  //Declare 7-segment shift register pins
#define SEGCLOCK 8
#define SEGLATCH 7

double vReal[SAMPLES];                                           //Create an array to hold the Real values
double vImag[SAMPLES];                                           //Create an array to hold the Imaginary values
arduinoFFT FFT = arduinoFFT(vReal, vImag, SAMPLES, SAMPLEFREQ);  //Give the FFT the required data

long period = 0;  //Spacing between samples
long time = 0;    //Time each sample was taken

void distanceTo(int distance) {  //Displays the proximity to being in-tune on a bargraph
  uint8_t value = 0;             //Creates the int that will be displayed on the bargraph
  if (distance > 7)              //Determes the corresponding Bargraph LED to the distance in hz
    value = 0x2;                 //Which LED is turned on on the Bargraph
  else if (distance > 4 && distance <= 7)
    value = 0x4;
  else if (distance > 1 && distance <= 4)
    value = 0x8;
  else if (distance < 2 && distance > -2)
    value = 0x10;
  else if (distance < 1 && distance >= (-4))
    value = 0x20;
  else if (distance < (-4) && distance >= (-7))
    value = 0x40;
  else
    value = 0x80;

  digitalWrite(BARLATCH, LOW);
  shiftOut(BARDATA, BARCLOCK, MSBFIRST, value);  //Shift out the LED values
  digitalWrite(BARLATCH, HIGH);
}

void display(uint8_t note) {  //shifts out the data for the 7-seg display
  digitalWrite(SEGLATCH, LOW);
  shiftOut(SEGDATA, SEGCLOCK, MSBFIRST, note);
  digitalWrite(SEGLATCH, HIGH);
}

void tune(int frequency) {                        //Determines the closest note to the freqency
  if (abs(frequency - E) < abs(frequency - A)) {  //Finds closest note
    display(letterE);                             //Display that note on the 7-seg. display
    distanceTo((frequency - E));                  //get the distance to the note and input it for the Bargraph
  } else if (abs(frequency - A) < abs(frequency - D)) {
    display(letterA);
    distanceTo((frequency - A));
  } else if (abs(frequency - D) < abs(frequency - G)) {
    display(letterD);
    distanceTo((frequency - D));
  } else {
    display(letterG);
    distanceTo((frequency - G));
  }
}

void setup() {
  Serial.begin(9600);  //Start Serial Monitor for debugging
  while (!Serial)
    ;                                            //Wait for Serial Communications to start
  period = round(1000000 * (1.0 / SAMPLEFREQ));  //Determine the spacing between samples

  pinMode(SEGDATA, OUTPUT);  //Declare the output for the 7seg display
  pinMode(SEGCLOCK, OUTPUT);
  pinMode(SEGLATCH, OUTPUT);
  pinMode(BARDATA, OUTPUT);  //Declare the output for the Bargraph
  pinMode(BARCLOCK, OUTPUT);
  pinMode(BARLATCH, OUTPUT);
}

void loop() {
  for (int i = 0; i < SAMPLES; i++) {              //Creates the loop to get the samples for the FFT
    time = micros();                                //Get current time when the sample happens
    vReal[i] = analogRead(AUDIO_PIN);                 //Sample the mic
    vImag[i] = 0;                                       //vImag always is 0
    while (micros() < (time + period));                                                  //Wait for proper spacing between samples
  }

  FFT.DCRemoval();                                  //Removes the DC offset for the FFT
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  //Make the sample data a repeating sequence for the FFT
  FFT.Compute(FFT_FORWARD);                         //Computes the actual FFT
  FFT.ComplexToMagnitude();                         //Converts complex numbers to magnitudes
  int frequency = FFT.MajorPeakParabola();          //Outputs the most dominant frequency in hz

  if (frequency > LOWEND) {  //Filters out noise (high end filter is done with SAMPLEFREQ)
    tune(frequency);         //Calls the tune function and tunes
  }
}