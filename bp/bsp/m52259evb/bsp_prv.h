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
*   Private BSP definitions for Essensys SC944D (m52259evb).
*
*END************************************************************************/

#ifndef _bsp_prv_h_
#define _bsp_prv_h_

#ifdef __cplusplus
extern "C" {
#endif

extern void mcf5225_init(void);

#if BSPCFG_ENABLE_CPP
extern void __cpp_init(void);
#endif

#if MQX_EXIT_ENABLED
extern void _bsp_exit_handler(void);
#endif
extern void _bsp_timer_isr(void *);

extern MCF52XX_UART_SERIAL_INIT_STRUCT  _bsp_uart0_init;
extern MCF52XX_UART_SERIAL_INIT_STRUCT  _bsp_uart1_init;
extern MCF52XX_UART_SERIAL_INIT_STRUCT  _bsp_uart2_init;

extern MCF52XX_I2C_INIT_STRUCT _bsp_i2c0_init;

extern MCF5XXX_QSPI_INIT_STRUCT _bsp_qspi0_init;

extern const FLASHX_FILE_BLOCK _bsp_flashx_file_blocks[];
extern const FLASHX_INIT_STRUCT _bsp_flashx_init;

#ifdef __cplusplus
}
#endif

#endif
/* EOF */
