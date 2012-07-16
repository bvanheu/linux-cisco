#ifndef _CRC16_H_
#define _CRC16_H_

#include <stdint.h>
#include <sys/types.h>

uint16_t crc16_ccitt(const uint8_t *buffer, size_t size, uint16_t remainder);

#endif
