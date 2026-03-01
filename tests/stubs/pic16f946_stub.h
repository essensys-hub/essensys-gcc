#ifndef PIC16F946_STUB_H
#define PIC16F946_STUB_H
#include <stdint.h>

/* Registres PIC simules */
static uint8_t PORTA, PORTB, PORTC, PORTD, PORTE, PORTF, PORTG;
static uint8_t TRISA, TRISB, TRISC, TRISD, TRISE, TRISF, TRISG;
static uint8_t LATA, LATB, LATC, LATD, LATE, LATF, LATG;
static uint8_t TMR0, TMR1L, TMR1H, TMR2;
static uint8_t INTCON, PIR1, PIE1;
static uint8_t SSPBUF, SSPCON, SSPSTAT, SSPADD;
static uint8_t PR2, CCP1CON, CCPR1L;
static uint8_t ADRESH, ADRESL, ADCON0, ADCON1;

/* Registres supplementaires utilises par gestionsorties.c */
static uint8_t CCP2CON, CCPR2L;
static uint8_t ANSEL, LCDCON, OSCCON, OPTION_REG, LVDCON;
static uint8_t T2CON, T0IF, T0IE, TMR2IF, TMR2IE;
static uint8_t SSPIF, SSPIE, GIE, PEIE, IRVST, LVDIF, LVDIE;

#define __interrupt(x)
#define __bit uint8_t
#define CLRWDT() ((void)0)
#define NOP() ((void)0)
#define di() ((void)0)
#define ei() ((void)0)

/* XC8 built-in stubs */
#define __CONFIG(x)
#define eeprom_read(a) ((unsigned char)0)
#define eeprom_write(a, d) ((void)0)

#endif
