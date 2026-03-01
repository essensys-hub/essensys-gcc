/*HEADER**********************************************************************
*
* Copyright 2008 Freescale Semiconductor, Inc.
*
* This software is owned or controlled by Freescale Semiconductor.
* Use of this software is governed by the Freescale MQX RTOS License
* distributed with this Material.
* See the MQX_RTOS_LICENSE file distributed for more details.
*
* Brief License Summary:
* This software is provided in source form for you to use free of charge,
* but it is not open source software. You are allowed to use this software
* but you cannot redistribute it or derivative works of it in source form.
* The software may be used only in connection with a product containing
* a Freescale microprocessor, microcontroller, or digital signal processor.
* See license agreement file for full license terms including other
* restrictions.
*****************************************************************************
*
* Comments:
*
*   Board-specific definitions for the Essensys SC944D (m52259evb).
*   Adapted from twrmcf52259 (Tower board) for the SC944D hardware:
*     - MCF52259CAG80 @ 80 MHz, 25 MHz crystal
*     - PHY: DP83640TVV (RMII)
*     - UART0: touchscreen 9600 8N1 half-duplex
*     - UART1: TeleInfo 1200 7E1
*     - UART2: EspionRS debug 9600 8N1
*     - I2C0: BA bus, polled master, 50 kHz
*     - SPI CS0: EEPROM 25AA02E48T (MAC), CS2: Flash SST25VF016B (OTA)
*     - ADC: AIN0 (VBAT), AIN5 (leak1), AIN6 (leak2)
*     - PWM ch4: siren
*
*END************************************************************************/
#ifndef _m52259evb_h_
#define _m52259evb_h_ 1

#include "mqx_cnfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------
**                  HARDWARE INITIALIZATION DEFINITIONS
*/
#define BSP_M52259EVB 1

/*
** PROCESSOR MEMORY BOUNDS
*/
extern unsigned char __INTERNAL_SRAM_BASE[],  __INTERNAL_SRAM_SIZE[];
extern unsigned char __INTERNAL_FLASH_BASE[], __INTERNAL_FLASH_SIZE[];
extern unsigned char __EXTERNAL_MRAM_BASE[], __EXTERNAL_MRAM_SIZE[];
extern unsigned char __EXTERNAL_MRAM_ROM_BASE[],  __EXTERNAL_MRAM_ROM_SIZE[];
extern unsigned char __EXTERNAL_MRAM_RAM_BASE[],  __EXTERNAL_MRAM_RAM_SIZE[];
extern unsigned char __IPSBAR[];

#define BSP_IPSBAR                  ((uint32_t)__IPSBAR)
#define BSP_INTERNAL_SRAM_BASE      ((void *)__INTERNAL_SRAM_BASE)
#define BSP_INTERNAL_SRAM_SIZE      ((uint32_t)__INTERNAL_SRAM_SIZE)
#define BSP_INTERNAL_FLASH_BASE     ((void *)__INTERNAL_FLASH_BASE)
#define BSP_INTERNAL_FLASH_SIZE     (0x00080000)
#define BSP_INTERNAL_FLASH_SECTOR_SIZE  (0x1000)
#define BSP_EXTERNAL_MRAM_BASE      ((void *)__EXTERNAL_MRAM_BASE)
#define BSP_EXTERNAL_MRAM_SIZE      ((uint32_t)__EXTERNAL_MRAM_SIZE)
#define BSP_EXTERNAL_MRAM_ROM_BASE  ((void *)__EXTERNAL_MRAM_ROM_BASE)
#define BSP_EXTERNAL_MRAM_ROM_SIZE  ((uint32_t)__EXTERNAL_MRAM_ROM_SIZE)
#define BSP_EXTERNAL_MRAM_RAM_BASE  ((void *)__EXTERNAL_MRAM_RAM_BASE)
#define BSP_EXTERNAL_MRAM_RAM_SIZE  ((uint32_t)__EXTERNAL_MRAM_RAM_SIZE)

/* Enable modification of flash configuration bytes during loading for flash targets */
#ifndef BSPCFG_ENABLE_CFMPROTECT
    #define BSPCFG_ENABLE_CFMPROTECT        1
#endif

/*
** The timer MQX uses
*/
#define BSP_FIRST_TIMER                               (0) /* PIT0 */
#define BSP_LAST_TIMER                                (1) /* PIT1 */
#define BSP_TIMER                                     BSP_FIRST_TIMER
#define BSP_ADC_TIMER                                 BSP_LAST_TIMER

/*
** The clock tick rate in number of ticks per second
*/
#ifndef BSP_ALARM_FREQUENCY
#define BSP_ALARM_FREQUENCY                           (200)
#endif

#define BSP_ALARM_RESOLUTION                          (1000 / BSP_ALARM_FREQUENCY)

/*
** CLKIN speed — 25 MHz external oscillator, PLL to 80 MHz
*/
#define BSP_SYSTEM_CLOCK                              (80000000L)
#define BSP_BUS_CLOCK                                 (BSP_SYSTEM_CLOCK/2)

#define SYSTEM_CLOCK                    (BSP_SYSTEM_CLOCK/1000000)
#define SYSTEM_CLK_PERIOD               (1000000000/(BSP_SYSTEM_CLOCK))

/*
** Core Watchdog Control Register (CWCR)
*/
#define BSP_WATCHDOG_DISABLED   0
#define BSP_WATCHDOG_RESET      1
#define BSP_WATCHDOG_INTERRUPT  2
#define BSP_WATCHDOG_DELAY      7
#define BSP_WATCHDOG_INITIALIZATION BSP_WATCHDOG_INTERRUPT

/*
** Timer interrupt levels and vectors
*/
#define BSP_PIT0_INT_LEVEL                            (2)
#define BSP_PIT0_INT_SUBLEVEL                         (4)
#define BSP_PIT1_INT_LEVEL                            (2)
#define BSP_PIT1_INT_SUBLEVEL                         (3)
#define BSP_PIT0_INT_VECTOR                           (MCF5225_INT_PIT0)
#define BSP_PIT1_INT_VECTOR                           (MCF5225_INT_PIT1)

#define BSP_TIMER_INTERRUPT_LEVEL                     (BSP_PIT0_INT_LEVEL)
#define BSP_TIMER_INTERRUPT_SUBLEVEL                  (BSP_PIT0_INT_SUBLEVEL)
#define BSP_TIMER_INTERRUPT_VECTOR                    (BSP_PIT0_INT_VECTOR)

#define BSP_ADC_TIMER_INTERRUPT_LEVEL                 (BSP_PIT1_INT_LEVEL)
#define BSP_ADC_TIMER_INTERRUPT_SUBLEVEL              (BSP_PIT1_INT_SUBLEVEL)
#define BSP_ADC_TIMER_INTERRUPT_VECTOR                (BSP_PIT1_INT_VECTOR)

#define BSP_ADC_INTERRUPT_LEVEL                       (BSP_ADC_TIMER_INTERRUPT_LEVEL + 1)
#define BSP_ADC_INTERRUPT_SUBLEVEL                    (2)
#define BSP_ADC_INTERRUPT_VECTOR                      (MCF5225_INT_ADC_ADCA)

/*
** RTC interrupt
*/
#define BSP_RTC_INT_LEVEL                          (2)
#define BSP_RTC_INT_SUBLEVEL                       (4)
#define BSP_RTC_PRESCALER_GOCU                     (0x0000)
#define BSP_RTC_PRESCALER_GOCL                     (0x2000)

/*----------------------------------------------------------------------
**                  I2C — SC944D uses I2C0 for BA bus at 50 kHz
*/
#define BSP_I2C_CLOCK                                 (BSP_SYSTEM_CLOCK)
#define BSP_I2C0_ADDRESS                              (0x6E)
#define BSP_I2C0_BAUD_RATE                            (50000)
#define BSP_I2C0_MODE                                 (I2C_MODE_MASTER)
#define BSP_I2C0_INT_LEVEL                            (1)
#define BSP_I2C0_INT_SUBLEVEL                         (0)
#define BSP_I2C0_RX_BUFFER_SIZE                       (64)
#define BSP_I2C0_TX_BUFFER_SIZE                       (64)

/*----------------------------------------------------------------------
**                  SPI — CS0: EEPROM, CS2: Flash OTA — 500 kHz
*/
#define BSP_QSPI_RX_BUFFER_SIZE                       (32)
#define BSP_QSPI_TX_BUFFER_SIZE                       (32)
#define BSP_QSPI_BAUDRATE                             (500000)
#define BSP_QSPI_INT_LEVEL                            (4)
#define BSP_QSPI_INT_SUBLEVEL                         (0)
#define BSP_SPI_MEMORY_CHANNEL                        0

/*----------------------------------------------------------------------
**                  Ethernet — DP83640TVV PHY, RMII
*/
#ifndef BSP_ENET0_PHY_ADDR
#define BSP_ENET0_PHY_ADDR                                  0
#endif

#define BSP_ENET0_PHY_MII_SPEED                            (2500000L)

/*----------------------------------------------------------------------
**                  GPIO — SC944D pin mapping
**
** DIN (inputs)
*/
#define BSP_SC944D_I_OUVERTURE_SIRENE_INT   (GPIO_PORT_AS | GPIO_PIN2)
#define BSP_SC944D_I_OUVERTURE_SIRENE_EXT   (GPIO_PORT_TE | GPIO_PIN5)
#define BSP_SC944D_I_OUVERTURE_PANNEAU      (GPIO_PORT_AN | GPIO_PIN7)
#define BSP_SC944D_I_DETECT_OUV             (GPIO_PORT_TF | GPIO_PIN0)
#define BSP_SC944D_I_DETECT_PRES1           (GPIO_PORT_TE | GPIO_PIN7)
#define BSP_SC944D_I_DETECT_PRES2           (GPIO_PORT_TE | GPIO_PIN6)
#define BSP_SC944D_I_PLUIE                  (GPIO_PORT_DD | GPIO_PIN3)
#define BSP_SC944D_I_SECTEUR_SYNCHRO        (GPIO_PORT_NQ | GPIO_PIN7)
#define BSP_SC944D_I_ETAT_ALIM              (GPIO_PORT_AN | GPIO_PIN0)
#define BSP_SC944D_I_VITESSE_VENT           (GPIO_PORT_TA | GPIO_PIN3)
#define BSP_SC944D_I_NEW_D5                 (GPIO_PORT_TH | GPIO_PIN3)
#define BSP_SC944D_I_NEW_D6                 (GPIO_PORT_TH | GPIO_PIN4)
#define BSP_SC944D_I_NEW_D7                 (GPIO_PORT_TH | GPIO_PIN5)

/*
** DOUT (outputs)
*/
#define BSP_SC944D_O_SIRENE_EXT             (GPIO_PORT_TC | GPIO_PIN0)
#define BSP_SC944D_O_15VSP_ALIM_BA          (GPIO_PORT_TE | GPIO_PIN2)
#define BSP_SC944D_O_LED_ETAT               (GPIO_PORT_TE | GPIO_PIN0)
#define BSP_SC944D_O_BATT_CTRL              (GPIO_PORT_TE | GPIO_PIN1)
#define BSP_SC944D_O_VANNE_ARROSAGE         (GPIO_PORT_DD | GPIO_PIN7)
#define BSP_SC944D_O_PRISE_SECURITE         (GPIO_PORT_DD | GPIO_PIN4)
#define BSP_SC944D_O_MACHINE_LAVER          (GPIO_PORT_DD | GPIO_PIN6)
#define BSP_SC944D_O_CUMULUS                 (GPIO_PORT_DD | GPIO_PIN5)
#define BSP_SC944D_O_FP_ZJ                  (GPIO_PORT_TA | GPIO_PIN0)
#define BSP_SC944D_O_FP_ZN                  (GPIO_PORT_DD | GPIO_PIN0)
#define BSP_SC944D_O_FP_SDB1                (GPIO_PORT_DD | GPIO_PIN1)
#define BSP_SC944D_O_FP_SDB2                (GPIO_PORT_DD | GPIO_PIN2)
#define BSP_SC944D_O_TELEINF_LED            (GPIO_PORT_TC | GPIO_PIN3)
#define BSP_SC944D_O_ECRAN_DIR              (GPIO_PORT_UA | GPIO_PIN3)
#define BSP_SC944D_O_DEBUG_J1               (GPIO_PORT_TF | GPIO_PIN6)
#define BSP_SC944D_O_DEBUG_J2               (GPIO_PORT_TF | GPIO_PIN5)
#define BSP_SC944D_O_DEBUG_J3               (GPIO_PORT_TF | GPIO_PIN4)
#define BSP_SC944D_O_DEBUG_J4               (GPIO_PORT_TF | GPIO_PIN3)
#define BSP_SC944D_O_DEBUG_J5               (GPIO_PORT_TF | GPIO_PIN2)

/*
** ADC channels — battery, leak sensors
*/
#define BSP_ADC_CH_VBAT                     (ADC_SOURCE_AN0)
#define BSP_ADC_CH_FUITE1                   (ADC_SOURCE_AN5)
#define BSP_ADC_CH_FUITE2                   (ADC_SOURCE_AN6)

/*----------------------------------------------------------------------
**                  FEC interrupt levels
*/
#define BSP_ENET_DEVICE_COUNT                     MCF5XXX_FEC_DEVICE_COUNT
#define BSP_FEC_INT_TX_LEVEL                          (4)
#define BSP_FEC_INT_TX_SUBLEVEL                       (1)
#define BSP_FEC_INT_RX_LEVEL                          (4)
#define BSP_FEC_INT_RX_SUBLEVEL                       (0)

#define BSP_DEFAULT_ENET_DEVICE  0
#define BSP_DEFAULT_ENET_OUI     { 0x00, 0x00, 0x5E, 0, 0, 0 }

/* EPORT interrupts — hardware-wired, do not change */
#define BSP_EPORT_EPFX_INT_SUBLEVEL_MIDPOINT          (3)
#define BSP_EPORT0_EPF1_INT_LEVEL                     (1)
#define BSP_EPORT0_EPF1_INT_SUBLEVEL                  (BSP_EPORT_EPFX_INT_SUBLEVEL_MIDPOINT)
#define BSP_EPORT0_EPF3_INT_LEVEL                     (3)
#define BSP_EPORT0_EPF3_INT_SUBLEVEL                  (BSP_EPORT_EPFX_INT_SUBLEVEL_MIDPOINT)
#define BSP_EPORT0_EPF5_INT_LEVEL                     (5)
#define BSP_EPORT0_EPF5_INT_SUBLEVEL                  (BSP_EPORT_EPFX_INT_SUBLEVEL_MIDPOINT)
#define BSP_EPORT0_EPF7_INT_LEVEL                     (7)
#define BSP_EPORT0_EPF7_INT_SUBLEVEL                  (BSP_EPORT_EPFX_INT_SUBLEVEL_MIDPOINT)

#ifndef BSPCFG_RX_RING_LEN
  #define BSPCFG_RX_RING_LEN             3
#endif

#ifndef BSPCFG_TX_RING_LEN
  #define BSPCFG_TX_RING_LEN             3
#endif

#ifndef BSPCFG_ENABLE_ENET_STATS
   #define BSPCFG_ENABLE_ENET_STATS  1
#endif

#ifndef BSPCFG_ENET_RESTORE
   #define BSPCFG_ENET_RESTORE  1
#endif

#ifndef BSPCFG_ENABLE_CPP
#define BSPCFG_ENABLE_CPP   0
#endif

/*----------------------------------------------------------------------
**                  INTERRUPT VECTOR TABLE
*/
typedef void (*vector_entry)(void);

extern vector_entry __VECTOR_TABLE_ROM_START[];
extern vector_entry __VECTOR_TABLE_RAM_START[];

#if MQX_ROM_VECTORS
    #define BSP_INTERRUPT_VECTOR_TABLE                    ((uint32_t)__VECTOR_TABLE_ROM_START)
#else
    #define BSP_INTERRUPT_VECTOR_TABLE                    ((uint32_t)__VECTOR_TABLE_RAM_START)
#endif

/*
** UART interrupt levels and vectors
*/
#define BSP_UART0_INT_LEVEL                           (5)
#define BSP_UART0_INT_SUBLEVEL                        (3)
#define BSP_UART1_INT_LEVEL                           (5)
#define BSP_UART1_INT_SUBLEVEL                        (2)
#define BSP_UART2_INT_LEVEL                           (5)
#define BSP_UART2_INT_SUBLEVEL                        (1)
#define BSP_UART0_INT_VECTOR                          (MCF5225_INT_UART0)
#define BSP_UART1_INT_VECTOR                          (MCF5225_INT_UART1)
#define BSP_UART2_INT_VECTOR                          (MCF5225_INT_UART2)

/*
** Interrupt vector range
*/
#ifndef BSP_FIRST_INTERRUPT_VECTOR_USED
    #define BSP_FIRST_INTERRUPT_VECTOR_USED           MCF5225_INT_FIRST_EXTERNAL
#endif

#ifndef BSP_LAST_INTERRUPT_VECTOR_USED
    #define BSP_LAST_INTERRUPT_VECTOR_USED            MCF5225_INT_FLEXCAN_BOFF_INT
#endif

/*----------------------------------------------------------------------
**                  IO DEVICE DRIVERS CONFIGURATION
**
** SC944D uses interrupt-driven UARTs, polled I2C, polled SPI.
** No USB, no LCD, no SD card, no touch screen, no FlexBus, no I2C1.
*/

#ifndef BSPCFG_ENABLE_IO_SUBSYSTEM
    #define BSPCFG_ENABLE_IO_SUBSYSTEM  1
#endif

/* Polled TTY — disabled, SC944D uses interrupt-driven */
#ifndef BSPCFG_ENABLE_TTYA
#define BSPCFG_ENABLE_TTYA  0
#endif

#ifndef BSPCFG_ENABLE_TTYB
#define BSPCFG_ENABLE_TTYB  0
#endif

#ifndef BSPCFG_ENABLE_TTYC
#define BSPCFG_ENABLE_TTYC  0
#endif

/* Interrupt-driven TTY — enabled for all three UARTs */
#ifndef BSPCFG_ENABLE_ITTYA
#define BSPCFG_ENABLE_ITTYA 1
#endif

#ifndef BSPCFG_ENABLE_ITTYB
#define BSPCFG_ENABLE_ITTYB 1
#endif

#ifndef BSPCFG_ENABLE_ITTYC
#define BSPCFG_ENABLE_ITTYC 1
#endif

/* No HW flow control signals on SC944D */
#ifndef BSPCFG_ENABLE_TTYA_HW_SIGNALS
#define BSPCFG_ENABLE_TTYA_HW_SIGNALS 0
#endif

#ifndef BSPCFG_ENABLE_TTYB_HW_SIGNALS
#define BSPCFG_ENABLE_TTYB_HW_SIGNALS 0
#endif

#ifndef BSPCFG_ENABLE_TTYC_HW_SIGNALS
#define BSPCFG_ENABLE_TTYC_HW_SIGNALS 0
#endif

/* UART0: touchscreen — 9600 baud */
#ifndef BSPCFG_UART0_BAUD_RATE
#define BSPCFG_UART0_BAUD_RATE 9600
#endif

/* UART1: TeleInfo Linky — 1200 baud */
#ifndef BSPCFG_UART1_BAUD_RATE
#define BSPCFG_UART1_BAUD_RATE 1200
#endif

/* UART2: EspionRS debug — 9600 baud */
#ifndef BSPCFG_UART2_BAUD_RATE
#define BSPCFG_UART2_BAUD_RATE 9600
#endif

#ifndef BSPCFG_UART0_QUEUE_SIZE
#define BSPCFG_UART0_QUEUE_SIZE 64
#endif

#ifndef BSPCFG_UART1_QUEUE_SIZE
#define BSPCFG_UART1_QUEUE_SIZE 64
#endif

#ifndef BSPCFG_UART2_QUEUE_SIZE
#define BSPCFG_UART2_QUEUE_SIZE 64
#endif

#ifndef BSPCFG_UART0_STATION_ADDR
#define BSPCFG_UART0_STATION_ADDR      (0)
#endif

#ifndef BSPCFG_UART1_STATION_ADDR
#define BSPCFG_UART1_STATION_ADDR      (1)
#endif

#ifndef BSPCFG_UART2_STATION_ADDR
#define BSPCFG_UART2_STATION_ADDR      (2)
#endif

/* I2C0: BA bus — polled */
#ifndef BSPCFG_ENABLE_I2C0
#define BSPCFG_ENABLE_I2C0 1
#endif

/* I2C1: not used on SC944D */
#ifndef BSPCFG_ENABLE_I2C1
#define BSPCFG_ENABLE_I2C1 0
#endif

/* SPI: polled (EEPROM + Flash) */
#ifndef BSPCFG_ENABLE_SPI0
#define BSPCFG_ENABLE_SPI0      1
#endif

#ifndef BSPCFG_ENABLE_ISPI0
#define BSPCFG_ENABLE_ISPI0     0
#endif

/* GPIO driver */
#ifndef BSPCFG_ENABLE_GPIODEV
#define BSPCFG_ENABLE_GPIODEV  1
#endif

/* RTC */
#ifndef BSPCFG_ENABLE_RTCDEV
#define BSPCFG_ENABLE_RTCDEV  1
#endif

/* Internal flash */
#ifndef BSPCFG_ENABLE_FLASHX
#define BSPCFG_ENABLE_FLASHX        1
#endif

/* ADC */
#ifndef BSPCFG_ENABLE_ADC
#define BSPCFG_ENABLE_ADC 1
#endif

/* Not used on SC944D */
#ifndef BSPCFG_ENABLE_TCHSRES
#define BSPCFG_ENABLE_TCHSRES 0
#endif

#ifndef BSPCFG_ENABLE_PCFLASH
#define BSPCFG_ENABLE_PCFLASH 0
#endif

/*----------------------------------------------------------------------
**                  DEFAULT MQX INITIALIZATION DEFINITIONS
*/
extern unsigned char __KERNEL_DATA_START[];
extern unsigned char __KERNEL_DATA_END[];
extern unsigned char __DEFAULT_PROCESSOR_NUMBER[];
extern unsigned char __DEFAULT_INTERRUPT_STACK_SIZE[];

#define BSP_DEFAULT_START_OF_KERNEL_MEMORY                  ((void *)__KERNEL_DATA_START)
#define BSP_DEFAULT_END_OF_KERNEL_MEMORY                    ((void *)__KERNEL_DATA_END)
#define BSP_DEFAULT_PROCESSOR_NUMBER                        ((uint32_t)__DEFAULT_PROCESSOR_NUMBER)

#ifndef BSP_DEFAULT_INTERRUPT_STACK_SIZE
    #define BSP_DEFAULT_INTERRUPT_STACK_SIZE                ((uint32_t)__DEFAULT_INTERRUPT_STACK_SIZE)
#endif

#ifndef BSP_DEFAULT_MQX_HARDWARE_INTERRUPT_LEVEL_MAX
    #define BSP_DEFAULT_MQX_HARDWARE_INTERRUPT_LEVEL_MAX    (6L)
#endif

#ifndef BSP_DEFAULT_MAX_MSGPOOLS
    #define BSP_DEFAULT_MAX_MSGPOOLS                        (16L)
#endif

#ifndef BSP_DEFAULT_MAX_MSGQS
    #define BSP_DEFAULT_MAX_MSGQS                           (16L)
#endif

/* Default I/O: EspionRS debug on UART2 */
#ifndef BSP_DEFAULT_IO_CHANNEL
    #if BSPCFG_ENABLE_ITTYC
        #define BSP_DEFAULT_IO_CHANNEL                      "ittyc:"
        #define BSP_DEFAULT_IO_CHANNEL_DEFINED
    #elif BSPCFG_ENABLE_TTYC
        #define BSP_DEFAULT_IO_CHANNEL                      "ttyc:"
        #define BSP_DEFAULT_IO_CHANNEL_DEFINED
    #else
        #define BSP_DEFAULT_IO_CHANNEL                      NULL
    #endif
#else
    #undef  BSP_DEFAULT_IO_CHANNEL_DEFINED
    #define BSP_DEFAULT_IO_CHANNEL_DEFINED
#endif

#ifndef BSP_DEFAULT_IO_OPEN_MODE
    #define BSP_DEFAULT_IO_OPEN_MODE                        (void *) (IO_SERIAL_XON_XOFF | IO_SERIAL_TRANSLATION | IO_SERIAL_ECHO)
#endif

/*
** CFM internal flash
*/
#if BSPCFG_ENABLE_FLASHX
    #ifndef BSPCFG_FLASHX_SIZE
    #define BSPCFG_FLASHX_SIZE      0x4000
    #endif
#else
    #undef  BSPCFG_FLASHX_SIZE
    #define BSPCFG_FLASHX_SIZE      0x0
#endif

#ifdef __cplusplus
}
#endif

#endif
/* EOF */
