#ifndef GD25Q64C_H_
#define GD25Q64C_H_

int gd25q64c_lock_otp_register(const struct device *dev, uint8_t reg_idx);
int gd25q64c_erase_otp_register(const struct device *dev, uint8_t reg_idx);
int gd25q64c_program_otp_register(const struct device *dev, uint8_t reg_idx,
				  size_t addr, uint8_t *data, size_t size);
int gd25q64c_read_otp_register(const struct device *dev, uint8_t reg_idx,
			       uint32_t addr, uint8_t *buf, size_t size);


#endif /* GD25Q64C_H_ */
