#include <avr/io.h>                        // include the avr defines

; ************************************************************************
; *************************** COMPILER DEFINES ***************************
; ************************************************************************

.global main                              ; shows the compiler the main is here 
.equ RESET_ISR, 0x0000                    ; location of the Reset ISR
.equ OVF_ISR, 0x001A                      ; location of timer1 overflow ISR
.equ SLEEP_MODE, 0 << SM0                 ; chooses idle sleep mode
.equ SLEEP_ENABLE, 1 << SE                ; enables sleep (done right before entering sleep mode)
.equ DISABLED_PERIPHERALS, 1 << PRTWI | 1 << PRTIM2 | 1 << PRTIM0 | 1 << PRSPI ; disables I2C, Timer/Counter 0 & 2, SPI
.equ CLK_PRESCALLER, 4 << CS10            ; 254 Prescale
.equ OVF_INTERRUPT_EN, 1 << TOIE1         ; enable the overflow interrupt
.equ ADC_REFS, 1 << REFS0                 ; set the reference to AVcc
.equ ADC_ALIN, 1 << ADLAR                 ; left align ADC output
.equ ADC_PRESCALER, 6 << ADPS0            ; 64 prescale
.equ ADC_MUX, 3 << MUX0                   ; ADC3 (A3)
.equ ADC_ENABLE, 1 << ADEN                ; enables the ADC moduale
.equ ADC_START, 1 << ADSC                 ; start the ADC
.equ ADC_OUTPUT_LENGTH, 10                ; output length of ADC
.equ BAUD_PRESCALER, 103                  ; equivalent to a baud rate of 9600
.equ UART_ENABLE, 1 << RXEN0 | 1 << TXEN0 ; enables UART RX & TX
.equ UART_TX_LENGTH, 3 << UCSZ00          ; 8-bit long transmit
.equ EOL, 0x0A                            ; new line character

util  = 16                                ; utility register (VOLATILE)
util2 = 17                                ; utility register (VOLATILE)
bcd0  = 18                                ; holds digit 0 of BCD, register 0 of double dabble
bcd1  = 19                                ; holds digit 1 of BCD
bcd21 = 20                                ; holds digit 2 of BCD, register 1 of double dabble
bcd3  = 21                                ; holds digit 3 of BCD
txData = 22                               ; holds the data to be transmitted

.org RESET_ISR                            ; position for the reset in IVT
  jmp main                                ; call the main
.org OVF_ISR                              ; position for overflow on Interrupt Vector Table (IVT)
  jmp loop                                ; call the loop
.org _VECTORS_SIZE                        ; position beyond the IVT. See iom328p.h


; ************************************************************************
; ******************************** MAIN **********************************
; ************************************************************************

main:                                     ; the main fuction
setup:                                    ; label added for clarity
  cli                                     ; disable global interrupts during setup
  rcall powerConfig                       ; configure the sleep comand
  rcall clockConfig                       ; configure the Timer/Counter1
  rcall ADCConfig                         ; configure the ADC
  rcall UARTConfig                        ; configure the UART
  sei                                     ; enable gloabl interrupts
loop:                                     ; label called by the ISR when T/C2 OVF
  rcall analogRead                        ; read what is on A3
  rcall convertToBCD                      ; convert to BCD
  rcall convertToASCI                     ; convert BCD to ASCI
  mov txData, bcd3                        ; put r11 in r22
  rcall transmit                          ; transmit r22
  mov txData, bcd21                       ; put r20 in r22
  rcall transmit                          ; transmit r22
  mov txData, bcd1                        ; put r19 in r22
  rcall transmit                          ; transmit r22
  mov txData, bcd0                        ; put r18 in r22
  rcall transmit                          ; transmit r22
  ldi txData, EOL                         ; put EOL in r22
  rcall transmit                          ; transmit r22
  lds util, SMCR                          ; load the SMCR to r16
  ori util, SLEEP_ENABLE                  ; OR the sleep enable bit in r16
  sts SMCR, util                          ; enable the sleep before sleeping (recommended)
  sleep                                   ; enter power save mode (woken by the timer overflow interrupt)
  nop                                     ; gives the controller more time to boot
ret

; ************************************************************************
; ******************************** CONFIG ********************************
; ************************************************************************

; Configures the Sleep function
; PRECONDITION: r16 is volatile
; POSTCONDITION: A sleep command enters power save mode (awaken by TC2 OVF)
powerConfig:
  ldi util, SLEEP_MODE                    ; load the sleep mode into r16
  sts SMCR, util                          ; load r16 (power-save)
  ldi util, DISABLED_PERIPHERALS          ; load the disabled into r16
  sts PRR, util                           ; load r16 (Disable: TWI, T/C 0,2, SPI)
ret

; Configures the Timer/Counter2
; PRECONDITION: r16 is volatile
; POSTCONDITION: Timer/Counter2 prescalled by 1024, enables stack overflow interrupt
clockConfig:
  ldi util, CLK_PRESCALLER                ; load the prescalling value in r16
  sts TCCR1B, util                        ; enable the prescaller
  ldi util, OVF_INTERRUPT_EN              ; load the interrupt enable in r16
  sts TIMSK1, util                        ; enable overflow interrupt
ret

; Configures the ADC
; PRECONDITION: r16 is volatile
; POSTCONDITION: ADC enabled, no flags, no prescalling, ADLR 1, using AVcc
ADCConfig:
  ldi util, ADC_REFS | ADC_ALIN | ADC_MUX ; AVcc, ADLR 1, ADC3
  sts ADMUX, util                         ; AVcc, ADLR 1, ADC3
  ldi util, ADC_ENABLE | ADC_PRESCALER    ; load the ADC enable into r16
  sts ADCSRA, util                        ; enable ADC, no flags, no prescalling
ret

; Configures the UART
; PRECONDITION: r16 is volatile
; POSTCONDITION: UART configured 8-bit frames, RX & TX enabled
UARTConfig:
  ldi util, BAUD_PRESCALER                ; load the baud in r16
  sts UBRR0L, util                        ; send it in
  ldi util, UART_ENABLE                   ; load the UART enable in r16
  sts UCSR0B, util                        ; enable UART
  ldi util, UART_TX_LENGTH                ; load the length of TX in r16
  sts UCSR0C, util                        ; 8-bits in a transmit
ret

; ************************************************************************
; ****************************** FUNCTIONS *******************************
; ************************************************************************

; Runs ADC on ADC3 (A3)
; PRECONDITION: Z, r16 must volatile
; POSTCONDITION Z will have the ADC result
analogRead:
  lds util, ADCSRA                        ; read the ADCSRA to r16 (keep the settings)
  ori util, ADC_START                     ; set the start bit (ADSC) in r16
  sts ADCSRA, util                        ; start the conversion
ADCStatus:                                ; label to loop to check ADCStatus
  lds util, ADCSRA                        ; read the ADCSRA to r16
  sbrs util, ADIF                         ; skip the next line when conversion complete (ADIF)
  rjmp ADCStatus                          ; check the ADCStatus again 
  lds ZL, ADCL                            ; load the low ADC to ZL
  lds ZH, ADCH                            ; load the high ADC to ZH
  sbr util, ADIF                          ; set the ADIF flag
  sts ADCSRA, util                        ; send in, clears ADIF
ret

; Converts BIN to BCD
; PRECONDITION: BIN must be in Z; r16, r17 must be volatile
; POSTCONDITION: BCD in nibbles of r18, r19
convertToBCD:
  ldi util2, ADC_OUTPUT_LENGTH            ; laod 10 into util2 (keeps track of number of cycles)
  rcall setupDoubleDabble                 ; shift over 3 since it is the minimum to make 5
DoubleDabbleLoop:                         ; label where to repeat the double dabble
  rcall checkandAdd                       ; check if bins >= 5, and add 3 if needed
  rcall shiftLeft                         ; shift the value left
  dec util2                               ; decrement the value of util2
  cpi util2, 0                            ; check if util2 = 0
  brne DoubleDabbleLoop                   ; jump back to do it again
ret

; Converts a BCD to an ASCI
; PRECONDITION: Each nibble of r18, r19 must have single digit number; 
; POSTCONDITION: r18:r21 have respective ascii characters
convertToASCI:
  ldi bcd1, 0xF0                          ; load 0xF0 for masking later
  ldi bcd3, 0xF0                          ; load 0xF0 for masking later
  and bcd1, bcd0                          ; AND bcd1 with bcd0 (result in bcd1)    
  andi bcd0, 0x0F                         ; clear the upper nibble of bcd0
  swap bcd1                               ; swap the nibbles of bcd1
  and bcd3, bcd21                         ; AND bcd3 with bcd21 (result in bcd3)
  andi bcd21, 0x0F                        ; clear the upper nibble of bcd21
  swap bcd3                               ; swap the nibbles of bcd3
  ori bcd0, 0x30                          ; adds 48 to bcd0
  ori bcd1, 0x30                          ; adds 48 to bcd1
  ori bcd21, 0x30                         ; adds 48 to bcd21
  ori bcd3, 0x30                          ; adds 48 to bcd3
ret

; Transmits r16 thru UART
; PRECONDITION: data is loaded in r22; r16 is volatile
; POSTCONDITION: r16 transmitted through UART
transmit:
  lds util, UCSR0A                        ; load UCSR0A to util
  sbrs util, UDRE0                        ; check if USART Transmit buffer is clear
  rjmp transmit                           ; go back and if the buffer is clear
  sts UDR0, txData                        ; put data (txData) into buffer, sends the data
ret


; ************************************************************************
; ***************************** SUBFUNCTIONS *****************************
; ************************************************************************

; 3 shifts of the double dabble to start
; PRECONDITION: Z has BIN; r16 is volatile
; POSTCONDITION: r18 has the first 3 bits
setupDoubleDabble:
  clr bcd0                                ; clear bcd0
  clr bcd21                               ; clear bcd21
repeatShift:                              ; label to repeat the shift
  lsl ZL                                  ; shift ZL left
  rol ZH                                  ; shift ZH left with carry
  rol bcd0                                ; shift bcd0 left with carry
  dec util2                               ; decrament util2
  cpi util2, (ADC_OUTPUT_LENGTH - 3)      ; check if util2 3 less (shifted 3 times)
  brne repeatShift                        ; go back to repeat
ret

; shifts the contents of Z, and bins
; PRECONDITION: Values in Z, and r18:r21 have required data
; POSTCONDITION: Values in Z and r18:r21 have been lsl w/ carry
shiftLeft:
  lsl ZH                                  ; shift ZH left (no value in ZL)
  rol bcd0                                ; shift bcd0 left with carry
  rol bcd21                               ; shift bcd21 left with carry
ret

; Checks if each bin is >= 5, and adds 3 if so
; PRECONDITION: BIN in r18, r19; r16 is volatile
; POSTCONDITION: if niblles in r18, r19 >= 5, +3
checkandAdd:
  swap bcd0                               ; swap the nibbles of bcd0
  ldi util, 0x0F                          ; load 0x0F into the second utility
  and util, bcd0                          ; AND util with bcd 0 for the hi nibble
  cpi util, 5                             ; compares bcd0 with 5
  brlo skip0                              ; skip the next instruction if less
  subi bcd0, -3                           ; adds 3 to bcd0
skip0:                                    ; label skipping the add 3
  swap bcd0                               ; swap the nibbles of bcd0
  ldi util, 0x0F                          ; load 0x0F into the second utility
  and util, bcd0                          ; AND util with bcd 0 for the lo nibble
  cpi util, 5                             ; compares bcd0 with 5
  brlo skip1                              ; skip the next instruction if less
  subi bcd0, -3                           ; adds 3 to bcd0
skip1:                                    ; label skipping the add 3
  ldi util, 0x0F                          ; load 0x0F into the second utility
  and util, bcd21                         ; AND util with bcd 1 for the lo nibble
  cpi util, 5                             ; compares bcd21 with 5
  brlo skip2                              ; skip the next instruction if less
  subi bcd21, -3                          ; adds 3 to bcd21
skip2:                                    ; label skipping the add 3
ret
