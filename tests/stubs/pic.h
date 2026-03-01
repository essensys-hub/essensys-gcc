#ifndef PIC_STUB_H
#define PIC_STUB_H
#include <stdint.h>

/*
 * PIC register stubs with extern linkage for multi-TU integration tests.
 * The test file provides actual definitions; slavenode.c gets extern access.
 * Contrast with pic16f946_stub.h which uses static (single-TU unit tests).
 */
extern uint8_t PORTA, PORTB, PORTC, PORTD, PORTE, PORTF, PORTG;
extern uint8_t TRISA, TRISB, TRISC, TRISD, TRISE, TRISF, TRISG;
extern uint8_t LATA, LATB, LATC, LATD, LATE, LATF, LATG;
extern uint8_t TMR0, TMR1L, TMR1H, TMR2;
extern uint8_t INTCON, PIR1, PIE1;
extern uint8_t SSPBUF, SSPCON, SSPSTAT, SSPADD;
extern uint8_t PR2, CCP1CON, CCPR1L;
extern uint8_t ADRESH, ADRESL, ADCON0, ADCON1;
extern uint8_t CCP2CON, CCPR2L;
extern uint8_t ANSEL, LCDCON, OSCCON, OPTION_REG, LVDCON;
extern uint8_t T2CON, T0IF, T0IE, TMR2IF, TMR2IE;
extern uint8_t SSPIF, SSPIE, GIE, PEIE, IRVST, LVDIF, LVDIE;

/* SSP control bits accessed as standalone variables by slavenode.c */
extern uint8_t SSPOV, WCOL, CKP;

#define __interrupt(x)
#define __bit uint8_t
#define CLRWDT() ((void)0)
#define NOP()    ((void)0)
#define di()     ((void)0)
#define ei()     ((void)0)
#define __CONFIG(x)
#define eeprom_read(a)    ((unsigned char)0)
#define eeprom_write(a,d) ((void)0)

#endif
