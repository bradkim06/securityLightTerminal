#ifndef __MSP430FLASHUTIL_H__
#define __MSP430FLASHUTIL_H__

#include "var_type.h"

// flash address
#define FLASH_SEGA_ADDR 0x1980
#define FLASH_SEGB_ADDR 0x1900
#define FLASH_SEGC_ADDR 0x1880
#define FLASH_SEGD_ADDR 0x1800
#define SYSRSTIV_ADDR 0x019E // 0x0180 + 1E

BOOL MSP430FLASH_erasePage(unsigned char *addr);
BOOL MSP430FLASH_read(unsigned char *addr, unsigned char *value,
		      unsigned short len);
BOOL MSP430FLASH_write(unsigned char *addr, unsigned char *buf,
		       unsigned short len);

#endif
