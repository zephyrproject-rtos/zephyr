#ifndef SPI_NOR_INTERNAL_H_
#define SPI_NOR_INTERNAL_H_

/* Indicates that an access command includes bytes for the address.
 * If not provided the opcode is not followed by address bytes.
 */
#define NOR_ACCESS_ADDRESSED BIT(0)

/* Indicates that addressed access uses a 24-bit address regardless of
 * spi_nor_data::flag_32bit_addr.
 */
#define NOR_ACCESS_24BIT_ADDR BIT(1)

/* Indicates that addressed access uses a 32-bit address regardless of
 * spi_nor_data::flag_32bit_addr.
 */
#define NOR_ACCESS_32BIT_ADDR BIT(2)

/* Indicates that an access command is performing a write.  If not
 * provided access is a read.
 */
#define NOR_ACCESS_WRITE BIT(7)


int spi_nor_wait_until_ready(const struct device *dev);
void spi_nor_acquire_device(const struct device *dev);
void spi_nor_release_device(const struct device *dev);
int spi_nor_access(const struct device *const dev, uint8_t opcode,
		   unsigned int access, off_t addr, void *data, size_t length);

#define spi_nor_cmd_read(dev, opcode, dest, length) \
	spi_nor_access(dev, opcode, 0, 0, dest, length)
#define spi_nor_cmd_addr_read(dev, opcode, addr, dest, length) \
	spi_nor_access(dev, opcode, NOR_ACCESS_ADDRESSED, addr, dest, length)
#define spi_nor_cmd_write(dev, opcode) \
	spi_nor_access(dev, opcode, NOR_ACCESS_WRITE, 0, NULL, 0)
#define spi_nor_cmd_write_data(dev, opcode, src, length) \
	spi_nor_access(dev, opcode, NOR_ACCESS_WRITE, 0, (void*)src, length)
#define spi_nor_cmd_addr_write(dev, opcode, addr, src, length) \
	spi_nor_access(dev, opcode, NOR_ACCESS_WRITE | NOR_ACCESS_ADDRESSED, \
		       addr, (void *)src, length)

#endif /* SPI_NOR_INTERNAL_H_ */
