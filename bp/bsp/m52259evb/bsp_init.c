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
*   Flash boot code for Essensys SC944D (m52259evb).
*   Initializes PLL from 25 MHz crystal to 80 MHz system clock,
*   disables watchdog, resets PHY.
*   No FlexBus/MRAM/LCD — SC944D does not use these.
*
*END************************************************************************/

#include "mqx.h"
#include "bsp.h"
#include "bsp_prv.h"

/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : mcf5225_init
* Returned Value   : void
* Comments :
*   Initialize PLL and basic hardware.
*
*   SC944D crystal: 25 MHz
*     CCHR = 4 → FREF = 25/5 = 5 MHz
*     MFD  = 6 → FSYS = 5 * 2*(6+2) = 80 MHz
*     RFD  = 0 → no post-divider
*
*   WARNING: Called before C runtime init. No global variables.
*
*END*----------------------------------------------------------------------*/
void mcf5225_init
   (
      void
   )
{
   VMCF5225_STRUCT_PTR reg_ptr = (VMCF5225_STRUCT_PTR)BSP_IPSBAR;
   uint8_t tmp_8[2];
   uint32_t i;

   reg_ptr->SCM.CWCR = 0;

   reg_ptr->GPIO.PDDPAR = MCF5225_GPIO_PDDPAR_PST;

   /*
   ** PLL configuration for 25 MHz crystal → 80 MHz system clock
   ** FREF = 25 MHz / (CCHR+1) = 25/5 = 5 MHz
   ** FSYS = FREF * 2 * (MFD+2) = 5 * 2 * 8 = 80 MHz
   */
   reg_ptr->CLK.OCLR  = 0xf0;
   reg_ptr->CLK.CCHR  = 4;
   reg_ptr->CLK.SYNCR = 0 |
      MCF5225_CLOCK_SYNCR_RFD(0) |
      MCF5225_CLOCK_SYNCR_MFD(6) |
      MCF5225_CLOCK_SYNCR_PLLMODE|
      MCF5225_CLOCK_SYNCR_PLLEN;

   while (!(reg_ptr->CLK.SYNSR & MCF5225_CLOCK_SYNSR_LOCK)) {}

   reg_ptr->CLK.CCLR  = 0;
   reg_ptr->CLK.SYNCR |= MCF5225_CLOCK_SYNCR_CLKSRC;

   while (!(reg_ptr->CLK.SYNSR & MCF5225_CLOCK_SYNSR_LOCK)) {}

   reg_ptr->SCM.RAMBAR = MCF5225_SCM_RAMBAR_BA((uint32_t)__INTERNAL_SRAM_BASE) | MCF5225_SCM_RAMBAR_BDE;

   /* PHY reset via RSTO — assert then deassert to reset DP83640TVV */
   tmp_8[0] = reg_ptr->GPIO.PTIPAR;
   tmp_8[1] = reg_ptr->GPIO.PTJPAR;
   reg_ptr->GPIO.PTIPAR = 0x00;
   reg_ptr->GPIO.PTJPAR = 0x00;
   reg_ptr->GPIO.DDRTI = 0xFF;
   reg_ptr->GPIO.DDRTJ = 0xFF;
   reg_ptr->GPIO.PORTTI = 0x00;
   reg_ptr->GPIO.PORTTJ = 0x00;
   reg_ptr->CCM.RCR |= MCF5225_CCM_RCR_FRCRSTOUT;

   for (i = 0; i < 10000; i++)
      _ASM_NOP();

   reg_ptr->CCM.RCR &= ~(MCF5225_CCM_RCR_FRCRSTOUT);

   for (i = 0; i < 1000000; i++)
      _ASM_NOP();

   reg_ptr->GPIO.PTIPAR = tmp_8[0];
   reg_ptr->GPIO.PTJPAR = tmp_8[1];
}

/* EOF */
