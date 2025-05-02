/* ================================================================
About:
Controls the speed of a motor and takes the input from a central
avionics computer.

Contents:
- Controls drone motors speeds thru the 3 phases
- Allows speed commands to be input thru 2 pins
*Tailor made for the MAVERICK Avionic Computer (ATTINY87)

Abreviations:
- BEMF: Back EMF (produced by motors to enable phase switching)

Contributers:
Goran Staznik (2025) -> Main Contributer
 ==================================================================
*/

// ================================================================
// ===                 USER DATA & VARIABLES                    ===
// ================================================================
//Vars and user defines

uint8_t bldc_step = 0, motor_speed = 100, speedBuffer, tick;

// ================================================================
// ===                Interrupt Service Routine                 ===
// ================================================================
// Digital interrupt & Analog comparator ISRs

ISR(PCINT0_vect) //ISR every time there is a SCL
  speed_com()  //write that down write that down...
}

ISR(ANALOG_COMP_vect) { //When a zero crossing happens
  for (uint8_t i = 0; i < 10; i++) { // BEMF debounce
    if (bldc_step & 1)
      (ACSR & 0x20) ? i -= 1 : i -= 1;
  }
  bldc_call();  //step the motor
}

// ================================================================
// ===              Speed Communication Function                ===
// ================================================================
//Recives communication of motor speed (Recived in set of 8 ISRs)

speed_com() { //write down the incoming 2 wire
  PINA == (1 << PA4) ? (speedBuffer << 1) | 1 : (speedBuffer << 1) & 0;  //check if PA4 is high 
  tick++;                                                                //add 1 to tick
  tick == 8 ? motor_speed = speedBuffer: ;                               //if tick is 8, set the speed as the buffer
  tick %= 8;                                                             //reset tick to 0
}

// ================================================================
// ===                  Motor Move Function                     ===
// ================================================================
//Switches through the motor phases causing them to move


bldc_call() {      //does the housekeeping for moving 
  bldc_move();     //Call the bldc_move function
  bldc_step++;     //Increase the step counter
  bldc_step %= 6;  //keep it under 6
}

void bldc_move() {  // BLDC motor commutation function
  switch (bldc_step) {
    case 0:
      AH_BL();
      BEMF_C_RISING();
      break;
    case 1:
      AH_CL();
      BEMF_B_FALLING();
      break;
    case 2:
      BH_CL();
      BEMF_A_RISING();
      break;
    case 3:
      BH_AL();
      BEMF_C_FALLING();
      break;
    case 4:
      CH_AL();
      BEMF_B_RISING();
      break;
    case 5:
      CH_BL();
      BEMF_A_FALLING();
      break;
  }
}


// ================================================================
// ===                   MAIN CONSTRUCTOR                       ===
// ================================================================
//Sets up Initialize everything required

void setup() {
  DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB6) | (1 << PA0);  // Configure outputs 
  DDRA |= (1 << PA2);

  TCCR1A = 0x81;  //Phase correct PWM
  TCCR1B = 0x01;  //no prescaling

  PCICR |= 0x02;       //Enable interrupts on PORTA
  PCMSK0 |= (1 << 5);  // Set Interrupt flags on SCL (pin8)

  ACSR = 0x10;  // Disable and clear (flag bit) analog comparator interrupt

  for (int i = 5000; i > 100; i -= 20) {  //Gradually speeds up the motor untill there is enough BEMF
    delayMicroseconds(i);                 //the speed of the program here doesn't really matter
    bldc_call();
  }
}


// ================================================================
// ===                       MAIN LOOP                          ===
// ================================================================
//Loop the program

void loop() {
  OCR1A = motor_speed;  //PWM duty cycle
}

// ================================================================
// ===               Driver and BEMF Functions                  ===
// ================================================================
//The functions to switch phase and check BEMF

//BEMF Functions
void BEMF_A_RISING() {
  ADCSRB = (0 << ACME);  // Select AIN1 as comparator negative input
  ACSR |= 0x03;          // Set interrupt on rising edge
}
void BEMF_A_FALLING() {
  ADCSRB = (0 << ACME);  // Select AIN1 as comparator negative input
  ACSR &= ~0x01;         // Set interrupt on falling edge
}
void BEMF_B_RISING() {
  ADCSRA = (0 << ADEN);  // Disable the ADC module
  ADCSRB = (1 << ACME);
  ADMUX = 1;     // Select analog channel 1 as comparator negative input (PA1)
  ACSR |= 0x03;  //rising edge interrupt
}
void BEMF_B_FALLING() {
  ADCSRA = (0 << ADEN);  // Disable the ADC module
  ADCSRB = (1 << ACME);
  ADMUX = 1;  // Select analog channel 1 as comparator negative input (PA1)
  ACSR &= ~0x01;
}
void BEMF_C_RISING() {
  ADCSRA = (0 << ADEN);  // Disable the ADC module
  ADCSRB = (1 << ACME);
  ADMUX = 2;  // Select analog channel 2 as comparator negative input (PA2)
  ACSR |= 0x03;
}
void BEMF_C_FALLING() {
  ADCSRA = (0 << ADEN);  // Disable the ADC module
  ADCSRB = (1 << ACME);
  ADMUX = 2;  // Select analog channel 2 as comparator negative input (PA2)
  ACSR &= ~0x01;
}

//Driver Functions
void AH_BL() {
  PORTB = (1 << PB0) | (1 << PB3);
  TCCR1D = 0x01;  // Turn pin 20 (PB0) (OC1AU) PWM ON (pin 12 (PB6) & pin 18 (PB2)  OFF)
}
void AH_CL() {
  PORTB = (1 << PB0);
  PORTA = (1 << PA2); 
  TCCR1D = 0x01;  // Turn pin 20 (PB0) (OC1AU) PWM ON (pin 12 (PB6) & pin 18 (PB2)  OFF)
}
void BH_CL() {
  PORTB = (1 << PB2);
  PORTA = (1 << PA2); 
  TCCR1D = 0x02;  // Turn pin 18 (PB2) (OC1AV) PWM ON (pin 12 (PB6) & pin 20 (PB0)  OFF)
}
void BH_AL() {
  PORTB = (1 << PB2) | (1 << PB1);
  TCCR1D = 0x02;  //Turn pin 18 (PB2) (OC1AV) PWM ON (pin 12 (PB6) & pin 20 (PB0)  OFF)
}
void CH_AL() {
  PORTB = (1 << PB6) | (1 << PB1);
  TCCR1D = 0x08;  //Turn pin 12 (PB6) (OC1AX) PWM ON (pin 18 (PB2)  & pin 20 (PB0)  OFF)
}
void CH_BL() {
  PORTB = (1 << PB6) | (1 << PB3);
  TCCR1D = 0x08;  //Turn pin 12 (PB6) (OC1AX) PWM ON (pin 18 (PB2)  & pin 20 (PB0)  OFF)
}