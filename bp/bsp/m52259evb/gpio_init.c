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
*   Board-specific pin initialization for the Essensys SC944D (m52259evb).
*   Configures UART, I2C, SPI, Ethernet, and ADC pin muxing.
*
*END************************************************************************/

#include <mqx.h>
#include <bsp.h>
#include "serl_mcf52xx.h"
#include "rtc.h"
#include "mcf52xx_rtc.h"

/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : _bsp_assert_rsto
* Returned Value   : -
* Comments         :
*    This function set or clear rsto signal
*
*END*----------------------------------------------------------------------*/
void _bsp_assert_rsto
(
   bool assert
)
{
   VMCF5225_STRUCT_PTR reg_ptr = (VMCF5225_STRUCT_PTR)BSP_IPSBAR;

   if(assert == TRUE)
   {
      reg_ptr->CCM.RCR |= MCF5225_CCM_RCR_FRCRSTOUT;
   }
   else
   {
      reg_ptr->CCM.RCR &= ~(MCF5225_CCM_RCR_FRCRSTOUT);
   }
}

/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : _bsp_serial_io_init
* Returned Value   : 0 for success, -1 for failure
* Comments         :
*    Pin muxing for UART0/1/2 on SC944D.
*    UART0 (PUA): touchscreen, UART1 (PUB): TeleInfo, UART2 (PUC): debug
*
*END*----------------------------------------------------------------------*/
_mqx_int _bsp_serial_io_init
(
   uint8_t dev_num
)
{
 VMCF5225_STRUCT_PTR reg_ptr = _PSP_GET_IPSBAR();

 switch (dev_num) {
     case 0:
        reg_ptr->GPIO.PUAPAR |= 0
           | MCF5225_GPIO_PUAPAR_TXD0
           | MCF5225_GPIO_PUAPAR_RXD0
           ;
        break;
     case 1:
        reg_ptr->GPIO.PUBPAR |= 0
           | MCF5225_GPIO_PUBPAR_TXD1
           | MCF5225_GPIO_PUBPAR_RXD1
           ;
        break;
   case 2:
        reg_ptr->GPIO.PUCPAR |= 0
           | MCF5225_GPIO_PUCPAR_TXD2
           | MCF5225_GPIO_PUCPAR_RXD2
           ;
      break;
  default:
     return -1;
 }

 return 0;
}

/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : _bsp_rtc_io_init
* Returned Value   : none
* Comments         :
*    RTC init — 32.768 kHz external crystal
*
*END*----------------------------------------------------------------------*/
void _bsp_rtc_io_init
(
   void
)
{
   VMCF5225_CLK_STRUCT_PTR clk = &(((VMCF5225_STRUCT_PTR)_PSP_GET_IPSBAR())->CLK);
   VMCF52XX_RTC_STRUCT_PTR rtc = _bsp_get_rtc_base_address ();
   clk->RTCCR = MCF5225_CLOCK_RTCCR_EXTALEN | MCF5225_CLOCK_RTCCR_REFS | MCF5225_CLOCK_RTCCR_LPEN | MCF5225_CLOCK_RTCCR_RTCSEL ;
   rtc->RTCGOCU = BSP_RTC_PRESCALER_GOCU;
   rtc->RTCGOCU = BSP_RTC_PRESCALER_GOCU;
   rtc->RTCGOCL = BSP_RTC_PRESCALER_GOCL;
   rtc->RTCGOCL = BSP_RTC_PRESCALER_GOCL;
   _rtc_init (NULL);
}

/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : _bsp_enet_io_init
* Returned Value   : none
* Comments         :
*    Ethernet RMII pin muxing for DP83640TVV PHY.
*    PTI/PTJ: FEC RMII data, PNQ: FEC management.
*
*END*----------------------------------------------------------------------*/
void _bsp_enet_io_init
(
   uint32_t  device
)
{
   VMCF5225_STRUCT_PTR mcf5225_ptr;

   if (device >= BSP_ENET_DEVICE_COUNT)
      return;

   mcf5225_ptr = _PSP_GET_IPSBAR();
   mcf5225_ptr->GPIO.PNQPAR = (mcf5225_ptr->GPIO.PNQPAR & ~(3 << 6 | 3 << 10)) | 0x880;
   mcf5225_ptr->GPIO.PTIPAR = 0xFF;
   mcf5225_ptr->GPIO.PTJPAR = 0xFF;
}

/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : _bsp_i2c_io_init
* Returned Value   : 0 for success, -1 for failure
* Comments         :
*    I2C0: PAS pins for BA bus (polled master, 50 kHz).
*    I2C1: not used on SC944D.
*
*END*----------------------------------------------------------------------*/
_mqx_int _bsp_i2c_io_init
(
   uint8_t dev_num
)
{
    VMCF5225_STRUCT_PTR reg_ptr = _PSP_GET_IPSBAR();

    switch (dev_num) {
      case 0:
         reg_ptr->GPIO.PASPAR = (reg_ptr->GPIO.PASPAR & ~0x0F) | 0x05;
         break;
      default:
         return -1;
    }
    return 0;
}

/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : _bsp_qspi_io_init
* Returned Value   : 0 for success, -1 for failure
* Comments         :
*    QSPI pin muxing. SC944D uses CS0 (EEPROM) and CS2 (Flash OTA).
*    PQSPA0=QSPI_DOUT, PQSPA1=QSPI_DIN, PQSPA2=QSPI_CLK,
*    PQSPA3=QSPI_CS0 (EEPROM), PQSPA5=QSPI_CS2 (Flash)
*
*END*----------------------------------------------------------------------*/
_mqx_int _bsp_qspi_io_init
(
   uint8_t dev_num
)
{
    VMCF5225_STRUCT_PTR reg_ptr = _PSP_GET_IPSBAR();

    if (0 != dev_num) return -1;

    /* Enable QSPI_DOUT, QSPI_DIN, QSPI_CLK, QSPI_CS0 as primary function */
    reg_ptr->GPIO.PQSPAR &= (~ (MCF5225_GPIO_PQSPAR_PQSPA0(3) | MCF5225_GPIO_PQSPAR_PQSPA1(3) | MCF5225_GPIO_PQSPAR_PQSPA2(3) | MCF5225_GPIO_PQSPAR_PQSPA3(3)));
    reg_ptr->GPIO.PQSPAR |= (MCF5225_GPIO_PQSPAR_PQSPA0(1) | MCF5225_GPIO_PQSPAR_PQSPA1(1) | MCF5225_GPIO_PQSPAR_PQSPA2(1) | MCF5225_GPIO_PQSPAR_PQSPA3(1));

    /* Enable QSPI_CS2 (PQSPA5) for Flash OTA */
    reg_ptr->GPIO.PQSPAR &= ~MCF5225_GPIO_PQSPAR_PQSPA5(3);
    reg_ptr->GPIO.PQSPAR |= MCF5225_GPIO_PQSPAR_PQSPA5(1);

    return 0;
}

/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : _bsp_adc_io_init
* Returned Value   : 0 for success, -1 for failure
* Comments         :
*    ADC pin muxing. SC944D uses AN0 (VBAT), AN5 (leak1), AN6 (leak2).
*
*END*----------------------------------------------------------------------*/
_mqx_int _bsp_adc_io_init
(
   _mqx_uint source
)
{
    VMCF5225_GPIO_STRUCT_PTR gpio_ptr = _bsp_get_gpio_base_address();
    VMCF522XX_ADC_STRUCT_PTR adc_ptr = _bsp_get_adc_base_address();

    switch (source) {
    case ADC_SOURCE_AN0:
        gpio_ptr->PANPAR |= 0x01;
        adc_ptr->CTRL1 &= 0xffef;
        break;
    case ADC_SOURCE_AN1:
        gpio_ptr->PANPAR |= 0x02;
        adc_ptr->CTRL1 &= 0xffef;
        break;
    case ADC_SOURCE_AN2:
        gpio_ptr->PANPAR |= 0x04;
        adc_ptr->CTRL1 &= 0xffdf;
        break;
    case ADC_SOURCE_AN3:
        gpio_ptr->PANPAR |= 0x08;
        adc_ptr->CTRL1 &= 0xffdf;
        break;
    case ADC_SOURCE_AN4:
        gpio_ptr->PANPAR |= 0x10;
        adc_ptr->CTRL1 &= 0xffbf;
        break;
    case ADC_SOURCE_AN5:
        gpio_ptr->PANPAR |= 0x20;
        adc_ptr->CTRL1 &= 0xffbf;
        break;
    case ADC_SOURCE_AN6:
        gpio_ptr->PANPAR |= 0x40;
        adc_ptr->CTRL1 &= 0xff7f;
        break;
    case ADC_SOURCE_AN7:
        gpio_ptr->PANPAR |= 0x80;
        adc_ptr->CTRL1 &= 0xff7f;
        break;
    case ADC_SOURCE_AN0_AN1:
        gpio_ptr->PANPAR |= 0x03;
        adc_ptr->CTRL1 |= 0x0010;
        break;
    case ADC_SOURCE_AN2_AN3:
        gpio_ptr->PANPAR |= 0x0c;
        adc_ptr->CTRL1 |= 0x0020;
        break;
    case ADC_SOURCE_AN4_AN5:
        gpio_ptr->PANPAR |= 0x30;
        adc_ptr->CTRL1 |= 0x0040;
        break;
    case ADC_SOURCE_AN6_AN7:
        gpio_ptr->PANPAR |= 0xc0;
        adc_ptr->CTRL1 |= 0x0080;
        break;
    default:
        return -1;
    }

    return 0;
}

/* EOF */
