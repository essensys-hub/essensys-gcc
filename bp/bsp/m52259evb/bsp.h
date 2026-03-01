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
*   BSP master include for Essensys SC944D (m52259evb).
*
*END************************************************************************/
#ifndef _bsp_h_
#define _bsp_h_ 1

#include <bsp_rev.h>
#include <psp.h>
#include <fio.h>
#include <io.h>
#include <m52259evb.h>             /* SC944D board definitions */
#include <int_ctrl_mcf5225.h>
#include <timer_pit.h>
#include <enet.h>
#include <mcf5225_fec.h>
#include <phy_dp83xxx.h>           /* DP83640TVV on SC944D */
#include <serial.h>
#include <serl_mcf52xx.h>
#include <pcb.h>
#include <io_mem.h>
#include <io_null.h>
#include <lwgpio_mcf5225.h>
#include <lwgpio.h>
#include <io_gpio.h>
#include <i2c.h>
#include <i2c_mcf52xx.h>
#include <spi.h>
#include <spi_mcf5xxx_qspi.h>
#include <adc.h>
#include <flashx.h>
#include <flash_mcf52xx.h>
#include <flash_mcf5225.h>
#include <rtc_mcf52xx.h>
#include <rtc.h>

#define _mcf52xx_int_init     _mcf5225_int_init
#define _mcf52xx_int_unmask   _mcf5225_int_unmask
#define _mcf52xx_int_mask     _mcf5225_int_mask
#define _mcf52xx_int_mask_all _mcf5225_int_mask_all
#define _mcf52xx_swint_invoke _mcf5225_swint_invoke
#define _mcf52xx_swint_clear  _mcf5225_swint_clear

#define _bsp_int_init                              _mcf5225_int_init
#define _bsp_int_enable                            _mcf5225_int_unmask
#define _bsp_int_disable                           _mcf5225_int_mask
#define _software_interrupt_invoke(num)            _mcf5225_swint_invoke(num)
#define _software_interrupt_clear(num)             _mcf5225_swint_clear(num)

#define BSP_SOFTWARE_INTERRUPT_VECTOR              (MCF5225_INT_rsvd16)

extern void _bsp_assert_rsto( bool assert);
extern _mqx_int _bsp_serial_io_init (uint8_t);
extern void _bsp_rtc_io_init (void);
extern _mqx_int _bsp_i2c_io_init (uint8_t);
extern _mqx_int _bsp_qspi_io_init (uint8_t);
extern _mqx_int _bsp_adc_io_init(_mqx_uint);

extern void     _bsp_enet_io_init(_mqx_uint);
extern bool  _bsp_get_mac_address(uint32_t,uint32_t,_enet_address);
   
extern const ENET_IF_STRUCT ENET_0;

#endif
/* EOF */
