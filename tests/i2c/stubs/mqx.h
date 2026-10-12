/*
 * Stub hote minimal de <mqx.h> pour compiler le pilote I2C polled MCF52xx
 * (bp/drivers/i2c/i2c_pol_mcf52xx.c) hors cible. Feature
 * i2c-surge-protection-2026-10-007, Refs essensys-hub/essensys-board-SC944D#1.
 */
#ifndef ESS_STUB_MQX_H
#define ESS_STUB_MQX_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint32_t  _mqx_uint;
typedef int32_t   _mqx_int;
typedef uint32_t *_mqx_uint_ptr;
typedef uint32_t  _int_level;
typedef uint32_t  _int_priority;
typedef uint32_t  _mem_type;
#define _CODE_PTR_ *

#define MQX_OK                 0
#define MQX_OUT_OF_MEMORY      0x0004
#define DRIVER_ERROR_BASE      0x1000
#define I2C_ERROR_BASE         (DRIVER_ERROR_BASE | 0x0900)
#define IO_ERROR_INVALID_IOCTL_CMD 0x0A08
#define MEM_TYPE_IO_I2C_INFO_STRUCT 1

#include <ioctl.h>
#define IO_IOCTL_FLUSH_OUTPUT _IO(IO_TYPE_MQX,0x03)

#define TRUE  1
#define FALSE 0

void *_mem_alloc_system_zero(_mqx_uint size);
_mqx_uint _mem_free(void *ptr);
bool _mem_set_type(void *ptr, _mem_type type);
#endif
