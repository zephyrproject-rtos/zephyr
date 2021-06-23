#ifndef SPI_NOR_INTERNAL_H_
#define SPI_NOR_INTERNAL_H_

int spi_nor_wait_until_ready(const struct device *dev);
void spi_nor_acquire_device(const struct device *dev);
void spi_nor_release_device(const struct device *dev);
int spi_nor_access(const struct device *const dev, uint8_t opcode,
		   bool is_addressed, off_t addr, void *data, size_t length,
		   bool is_write, bool rx_dummy_byte);

#define spi_nor_cmd_read(dev, opcode, dest, length)			\
	spi_nor_access(dev, opcode, false, 0, dest, length, false, false)
#define spi_nor_cmd_addr_read(dev, opcode, addr, dest, length)		\
	spi_nor_access(dev, opcode, true, addr, dest, length, false, false)
#define spi_nor_cmd_write(dev, opcode)					\
	spi_nor_access(dev, opcode, false, 0, NULL, 0, true, false)
#define spi_nor_cmd_write_data(dev, opcode, src, length)		\
	spi_nor_access(dev, opcode, false, 0, (void *)src, length, true, false)
#define spi_nor_cmd_addr_write(dev, opcode, addr, src, length)		\
	spi_nor_access(dev, opcode, true, addr, (void *)src, length, true, false)

#endif /* SPI_NOR_INTERNAL_H_ */
