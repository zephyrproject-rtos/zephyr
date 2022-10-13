#ifndef _CRC32_H_
#define _CRC32_H_

void crc32_init(void);
void crc32_hash(const void *buf, int size);
void crc32_hash32(uint32_t val);
void crc32_hash16(uint16_t val);
uint32_t crc32_result(void);

#endif /* _CRC32_H_ */
