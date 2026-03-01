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
*   BSP initialization for Essensys SC944D (m52259evb).
*   Adapted from twrmcf52259 — removed USB, LCD, SD, touch screen,
*   FlexBus, I2C1, PCCard.
*
*END************************************************************************/

#include "mqx_inc.h"
#include "bsp.h"
#include "bsp_prv.h"
#include "rtc.h"

const char      *_mqx_bsp_revision = REAL_NUM_TO_STR(BSP_REVISION);
const char      *_mqx_io_revision  = REAL_NUM_TO_STR(IO_REVISION);

static void    _bsp_setup_watchdog(void);

int _bsp_pre_init(void) {
   KERNEL_DATA_STRUCT_PTR   kernel_data;
   uint32_t                  result;

   _mqx_set_cpu_type(MQX_CPU);

#if MQX_EXIT_ENABLED
   _mqx_set_exit_handler(_bsp_exit_handler);
#endif

   _mcf5225_initialize_support(0);

   _mcf5225_int_mask_all();

   _int_set_vector_table(BSP_INTERRUPT_VECTOR_TABLE);

   result =  _psp_int_init(BSP_FIRST_INTERRUPT_VECTOR_USED,
      BSP_LAST_INTERRUPT_VECTOR_USED);
   if (result != MQX_OK) {
      return result;
   }

   _time_set_timer_vector(BSP_TIMER_INTERRUPT_VECTOR);
   if (_int_install_isr(BSP_TIMER_INTERRUPT_VECTOR,
                        _bsp_timer_isr, NULL) == NULL)
   {
      return MQX_TIMER_ISR_INSTALL_FAIL;
   }

   _GET_KERNEL_DATA(kernel_data);

   kernel_data->TIMER_HW_REFERENCE = _pit_init_freq(BSP_TIMER, BSP_ALARM_FREQUENCY, BSP_SYSTEM_CLOCK/2, FALSE);

   _time_set_hwtick_function(_pit_get_hwticks, (void *)BSP_TIMER);
   _time_set_hwticks_per_tick(kernel_data->TIMER_HW_REFERENCE);
   _time_set_ticks_per_sec(BSP_ALARM_FREQUENCY);

   _pit_unmask_int(BSP_TIMER);

#if BSPCFG_ENABLE_CPP
   __cpp_init();
#endif

   return 0;
}

int _bsp_init(void) {
    uint32_t result;

#if BSPCFG_ENABLE_RTCDEV
   _bsp_rtc_io_init ();
#endif

#if BSPCFG_ENABLE_IO_SUBSYSTEM
   result = _io_init();
   if (result != MQX_OK) {
      return result;
   }

   /* Polled UART drivers */
#if BSPCFG_ENABLE_TTYA
   _mcf52xx_uart_serial_polled_install("ttya:", &_bsp_uart0_init,
      _bsp_uart0_init.QUEUE_SIZE);
#endif
#if BSPCFG_ENABLE_TTYB
   _mcf52xx_uart_serial_polled_install("ttyb:", &_bsp_uart1_init,
      _bsp_uart1_init.QUEUE_SIZE);
#endif
#if BSPCFG_ENABLE_TTYC
   _mcf52xx_uart_serial_polled_install("ttyc:", &_bsp_uart2_init,
      _bsp_uart2_init.QUEUE_SIZE);
#endif

   /* Interrupt-driven UART drivers */
#if BSPCFG_ENABLE_ITTYA
   _mcf52xx_uart_serial_int_install("ittya:", &_bsp_uart0_init,
      _bsp_uart0_init.QUEUE_SIZE);
#endif
#if BSPCFG_ENABLE_ITTYB
   _mcf52xx_uart_serial_int_install("ittyb:", &_bsp_uart1_init,
      _bsp_uart1_init.QUEUE_SIZE);
#endif
#if BSPCFG_ENABLE_ITTYC
   _mcf52xx_uart_serial_int_install("ittyc:", &_bsp_uart2_init,
      _bsp_uart2_init.QUEUE_SIZE);
#endif

   /* I2C0: BA bus */
#if BSPCFG_ENABLE_I2C0
   _mcf52xx_i2c_polled_install("i2c0:", &_bsp_i2c0_init);
#endif

   /* SPI: EEPROM + Flash OTA */
#if BSPCFG_ENABLE_SPI0
   _mcf5xxx_qspi_polled_install("spi0:", &_bsp_qspi0_init);
#endif

#if BSPCFG_ENABLE_ISPI0
   _mcf5xxx_qspi_int_install("ispi0:", &_bsp_qspi0_init);
#endif

   /* ADC */
#if BSPCFG_ENABLE_ADC
   _io_adc_install("adc:", NULL);
#endif

   _io_serial_default_init();

   /* GPIO */
#if BSPCFG_ENABLE_GPIODEV
   _io_gpio_install("gpio:");
#endif

   /* Internal flash */
#if BSPCFG_ENABLE_FLASHX
    _io_flashx_install("flashx:", &_bsp_flashx_init);
#endif

#endif /* BSPCFG_ENABLE_IO_SUBSYSTEM */

   _bsp_setup_watchdog();

   return 0;
}


void _bsp_exit_handler
   (
      void
   )
{
   uint16_t   temp;

   temp = _psp_set_sr(0x2700);
   _mcf5225_int_mask_all();
}


void _bsp_timer_isr
   (
      void   *dummy
   )
{
#if BSP_WATCHDOG_INITIALIZATION != BSP_WATCHDOG_DISABLED
   VMCF5225_STRUCT_PTR reg_ptr = _PSP_GET_IPSBAR();
   reg_ptr->SCM.CWSR = 0x55;
   reg_ptr->SCM.CWSR = 0xAA;
#endif

   _pit_clear_int(BSP_TIMER);
   _time_notify_kernel();
}


static void _bsp_setup_watchdog
   (
      void
   )
{
   VMCF5225_STRUCT_PTR reg_ptr = _PSP_GET_IPSBAR();

   reg_ptr->SCM.CWCR = 0;

   if (reg_ptr->SCM.CWCR & MCF5225_SCM_CWCR_CWTAVAL) {
      reg_ptr->SCM.CWCR = MCF5225_SCM_CWCR_CWTAVAL;
   }
   if (reg_ptr->SCM.CWCR & MCF5225_SCM_CWCR_CWTIC) {
      reg_ptr->SCM.CWCR = MCF5225_SCM_CWCR_CWTIC;
   }

   reg_ptr->SCM.CWSR = 0x55;
   reg_ptr->SCM.CWSR = 0xAA;

#if BSP_WATCHDOG_INITIALIZATION != BSP_WATCHDOG_DISABLED
   reg_ptr->SCM.CWCR = 0
# if BSP_WATCHDOG_INITIALIZATION == BSP_WATCHDOG_RESET
# error Watchdog resets immediately for some reason
      | MCF5225_SCM_CWCR_CWRI
# endif
      | MCF5225_SCM_CWCR_CWE
      | MCF5225_SCM_CWCR_CWT(BSP_WATCHDOG_DELAY);
#endif
}

/* EOF */
