/*HEADER**********************************************************************
*
* Copyright 2014 Freescale Semiconductor, Inc.
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
*   Ethernet FEC + DP83640TVV PHY initialization for Essensys SC944D.
*
*END************************************************************************/

#include "mqx.h"
#include "bsp.h"
#include "bsp_prv.h"
#include "enet.h"
#include "enetprv.h"
#include <mcf5xxx_fec_prv.h>

#include "phy_dp83xxx.h"
#include "fio.h"

const MCF5XXX_FEC_INIT_STRUCT MCF5XXX_FEC_device[MCF5XXX_FEC_DEVICE_COUNT] =  {
   { BSP_SYSTEM_CLOCK, BSP_FEC_INT_RX_LEVEL, BSP_FEC_INT_RX_SUBLEVEL, BSP_FEC_INT_TX_LEVEL, BSP_FEC_INT_TX_SUBLEVEL },
};

const ENET_IF_STRUCT ENET_0 = {
     &MCF5xxx_FEC_IF,
     &phy_dp83xxx_IF,
     MCF5225_FEC0,
     MCF5225_FEC0,
     BSP_ENET0_PHY_ADDR,
     BSP_ENET0_PHY_MII_SPEED
};

const ENET_PARAM_STRUCT ENET_default_params[BSP_ENET_DEVICE_COUNT] = {
    {
        &ENET_0,
        Auto_Negotiate,
        0,

        BSPCFG_TX_RING_LEN,
        BSPCFG_TX_RING_LEN,
        ENET_FRAMESIZE,

        BSPCFG_RX_RING_LEN,
        BSPCFG_RX_RING_LEN,
        ENET_FRAMESIZE,
        BSPCFG_RX_RING_LEN,

        0,
        0,
        NULL
    }
};

/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : _bsp_get_mac_address
* Returned Value   : uint32_t
* Comments         :
*    On SC944D, the MAC address is stored in EEPROM 25AA02E48T at offset
*    0xFA (6 bytes, EUI-48). This function returns a default OUI-based
*    address; the firmware reads the real MAC from EEPROM at runtime via SPI.
*
*END*----------------------------------------------------------------------*/

const _enet_address _enet_oui = BSP_DEFAULT_ENET_OUI;

bool _bsp_get_mac_address
   (
      uint32_t        device,
      uint32_t        value,
      _enet_address  address
   )
{
   if (device >= BSP_ENET_DEVICE_COUNT)
      return FALSE;

   address[0] = _enet_oui[0];
   address[1] = _enet_oui[1];
   address[2] = _enet_oui[2];
   address[3] = (value & 0xFF0000) >> 16;
   address[4] = (value & 0xFF00) >> 8;
   address[5] = (value & 0xFF);

   return TRUE;
}

/* EOF */
