#include <avr/io.h>			 //in turn includes 328p-specific predefines (iom328p.h) 

#define     DDR    DDRB-0x20      //avr-as requires relative addressing 
#define     PORT   PORTB-0x20     //ditto

.global setup             ;must be visible from outside this file
setup:                    ;one-time call from main.cpp
  sbi   DDR,0x0           ;set PORTB1 for output  (RED)
  sbi   DDR,0x1           ;set PORTB2 for output  (YELLOW)
  sbi   DDR,0x2           ;set PORTB3 for output  (GREEN)
  ldi   r16,0x1           ;load 0x1 into r16
  ret                     ;exit from function

.global loop              ;must be visible from outside this file
loop:                     ;repeated invocation from main.cpp
  out PORT, r16           ;whatever is in r16 on to the ports
  rcall delay             ;call the delay
  lsl r16                 ;(r16<<1)
  sbrc r16, 0x3           ;skip the next line if bit 4 is clear
  ldi r16, 0x1            ;rests r16
  ret                     ;exit from function

delay:
    ldi  r17, 0x1         ;holds the amount of times the delay loops
    sbrc r16, 0x1         ;skip the next line if bit 1 is clear
    ldi  r17, 0x10        ;put 8 in r17 so the loop stops after 1 cycle
Tp: ldi  r18, 2           ;5 second delay
    ldi  r19, 150         ;Assembly code auto-generated
    ldi  r20, 216         ;by utility from Bret Mulvey
    ldi  r21, 10
L1: dec  r21
    brne L1
    dec  r20
    brne L1
    dec  r19
    brne L1
    dec  r18
    brne L1
    lsl r17               ;(r17<<1)
    sbrs r17, 0x5         ;skip the next line if bit 5 is set
    rjmp Tp               ;goto Top
  ret                     ;exit from function