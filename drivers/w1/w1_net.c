/*
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief 1-Wire network related functions.
 *
 * The following procedures wrap basic w1 syscalls, they should be callable
 * from user mode as well as supervisor mode, therefore _ZEPHYR_SUPERVISOR__
 * is not defined for this file such that inline macros do not skip
 * the arch_is_user_context() check.
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/w1.h>

LOG_MODULE_REGISTER(w1, CONFIG_W1_LOG_LEVEL);

#define W1_SEARCH_DISCREPANCY_INIT 0
#define W1_SEARCH_LAST_SLAVE	   65
#define W1_SEARCH_NO_SLAVE	   66

/* @brief Search bus for next slave.
 *
 * This function searches the next 1-Wire slave on the bus.
 * It sets the found ROM and the last discrepancy in case more than one
 * slave took part in the search.
 * In case only one slave took part in the search, the discrepancy is set to
 * W1_SEARCH_LAST_SLAVE, and in case no slave participated in the search,
 * the discrepancy is set to W1_SEARCH_NO_SLAVE.
 *
 * The implementation is similar to suggested in the maxim application note 187.
 * The master reads the first ROM bit and its complementary value of all slaves.
 * Due to physical characteristics, the value received is a
 * logical AND of all slaves' 1st bit. Slaves only continue to
 * participate in the search procedure if the next bit the master sends matches
 * their own addresses' bit. This allows the master to branch through 64-bit
 * addresses in order to detect all slaves.

 * The 1st bit received is stored in bit 1 of rom_inv_64, the 2nd in bit 2 and so
 * on, until bit 64.
 * As a result, each byte of the ROM has the correct bit order, but the received
 * bytes (big-endian) stored in rom_inv_64 are in inverse byte order.
 *
 * Note: Filtering by families is currently not supported.
 *
 * @param dev              Pointer to the device structure for the w1 instance.
 * @param command          Command to chose between normal and alarm search.
 * @param family           This parameter is currently not supported.
 * @param last_discrepancy This must be set to W1_SEARCH_DISCREPANCY_INIT before
 *                         the first call, it carries the search progress for
 *                         further calls.
 * @param rom_inv_64       The found ROM: It must be set to zero before first
 *                         call and carries the last found ROM for furter calls.
 *                         The ROM is stored in inverse byte order.
 *
 * @retval 0      If successful.
 * @retval -errno Negative error code in case of 1-wire read/write error.
 */
static int search_slave(const struct device *dev, uint8_t command,
			 uint8_t family, size_t *last_discrepancy,
			 uint64_t *rom_inv_64)
{
	int ret;
	size_t next_discrepancy;
	bool last_id_bit;
	bool last_complement_id_bit;

	ARG_UNUSED(family);
	__ASSERT_NO_MSG(command == W1_CMD_SEARCH_ROM ||
			command == W1_CMD_SEARCH_ALARM);

	ret = w1_reset_bus(dev);
	if (ret < 0) {
		return ret;
	}
	if (ret == 0) {
		*last_discrepancy = W1_SEARCH_NO_SLAVE;
		return 0;
	}

	ret = w1_write_byte(dev, command);
	if (ret < 0) {
		return ret;
	}
	next_discrepancy = W1_SEARCH_LAST_SLAVE;

	for (size_t id_bit_nr = 1; id_bit_nr < W1_SEARCH_LAST_SLAVE; id_bit_nr++) {
		ret = w1_read_bit(dev);
		if (ret < 0) {
			return ret;
		}
		last_id_bit = (bool)ret;
		ret = w1_read_bit(dev);
		if (ret < 0) {
			return ret;
		}
		last_complement_id_bit = (bool)ret;

		if (last_id_bit && last_complement_id_bit) {
			/*
			 * No slave participating:
			 * We can stop following the branch.
			 */
			LOG_DBG("No slave paricipating");
			*last_discrepancy = W1_SEARCH_NO_SLAVE;
			return 0;
		} else if (last_id_bit != last_complement_id_bit) {
			/*
			 * All slaves connected have same ROM bit value:
			 * We can directly follow last_id_bit branch.
			 */
		} else {
			/*
			 * Discrepancy detected: bit value at id_bit_nr does
			 * not match for all slaves on the bus.
			 */
			if ((id_bit_nr > *last_discrepancy) ||
			    ((id_bit_nr < *last_discrepancy) &&
			     (*rom_inv_64 & BIT64(id_bit_nr - 1)))) {
				/*
				 * - id_bit_nr > last_discrepancy:
				 *     Start always w/ branch of 1s
				 * - id_bit_nr < last_discrepancy:
				 *     Follow same branch as before
				 */
				last_id_bit = true;
				next_discrepancy = id_bit_nr;
			} else {
				/*
				 * - id_bit_nr == last_discrepancy:
				 *     1-path already done, therefore go 0 path
				 * - id_bit_nr < last_discrepancy:
				 *     Follow same branch as before
				 */
			}
		}

		/*
		 * Send and store the chosen bit: all not matching slaves will
		 * no longer participate in this search until they are reset.
		 */
		ret = w1_write_bit(dev, last_id_bit);
		if (ret < 0) {
			return ret;
		}
		*rom_inv_64 &= ~BIT64(id_bit_nr - 1);
		*rom_inv_64 |= last_id_bit ? BIT64(id_bit_nr - 1) : 0;
	}

	*last_discrepancy = next_discrepancy;
	return 0;
}

int z_impl_w1_search_bus(const struct device *dev, uint8_t command,
			 uint8_t family, w1_search_callback_t callback,
			 void *user_data)
{
	size_t last_discrepancy = W1_SEARCH_DISCREPANCY_INIT;
	uint64_t found_rom_inv_64 = 0;
	struct w1_rom found_rom = { 0 };
	int found_cnt = 0;
	int ret;

	(void)w1_lock_bus(dev);

	do {
		ret = search_slave(dev, command, family, &last_discrepancy,
				    &found_rom_inv_64);
		if (ret < 0) {
			found_cnt = ret;
			break;
		}
		if (last_discrepancy == W1_SEARCH_NO_SLAVE) {
			break;
		}

		found_cnt++;
		/*
		 * ROM is stored in found_rom_inv_64 in "inverse byte order" =>
		 * Only big-endian targets need to swap, such that struct's
		 * bytes are stored in big-endian byte order.
		 */
		if (IS_ENABLED(CONFIG_BIG_ENDIAN)) {
			sys_memcpy_swap(&found_rom, &found_rom_inv_64, 8);
		} else {
			*(uint64_t *)&found_rom = found_rom_inv_64;
		}
		LOG_DBG("ROM found: nr %u, %016llx", found_cnt,
			w1_rom_to_uint64(&found_rom));

		if (callback != NULL) {
			callback(found_rom, user_data);
		}

	} while (last_discrepancy != W1_SEARCH_LAST_SLAVE);

	(void)w1_unlock_bus(dev);
	return found_cnt;
}

int w1_read_rom(const struct device *dev, struct w1_rom *rom)
{
	int ret;

	(void)w1_lock_bus(dev);
	ret = w1_reset_bus(dev);
	if (ret == 0) {
		ret = -ENODEV;
		goto out;
	}
	if (ret < 0) {
		goto out;
	}

	ret = w1_write_byte(dev, W1_CMD_READ_ROM);
	if (ret < 0) {
		goto out;
	}
	ret = w1_read_block(dev, (uint8_t *)rom, sizeof(struct w1_rom));
	if (ret < 0) {
		goto out;
	}
	if (w1_crc8((uint8_t *)rom, sizeof(struct w1_rom)) != 0) {
		ret = -EIO;
	}

out:
	(void)w1_unlock_bus(dev);
	return ret;
};

static int match_rom(const struct device *dev, const struct w1_slave_config *config)
{
	int ret;
	uint8_t cmd;

	if (!config->overdrive) {
		if (w1_configure(dev, W1_SETTING_SPEED, 0) < 0) {
			return -EIO;
		}
	}

	ret = w1_reset_bus(dev);
	if (ret == 0) {
		return -ENODEV;
	}
	if (ret < 0) {
		return ret;
	}

	cmd = config->overdrive ? W1_CMD_OVERDRIVE_MATCH_ROM : W1_CMD_MATCH_ROM;
	ret = w1_write_byte(dev, cmd);
	if (ret < 0) {
		return ret;
	}
	ret = w1_write_block(dev, (uint8_t *)&config->rom, 8);
	if (ret < 0) {
		return ret;
	}

	if (config->overdrive) {
		if (w1_configure(dev, W1_SETTING_SPEED, 1) < 0) {
			return -EIO;
		}
	}

	return 0;
};

int w1_match_rom(const struct device *dev, const struct w1_slave_config *config)
{
	int ret;

	(void)w1_lock_bus(dev);
	ret = match_rom(dev, config);
	(void)w1_unlock_bus(dev);
	return ret;
}

int w1_resume_command(const struct device *dev)
{
	int ret;

	(void)w1_lock_bus(dev);
	ret = w1_reset_bus(dev);
	if (ret == 0) {
		ret = -ENODEV;
		goto out;
	}
	if (ret < 0) {
		goto out;
	}

	ret = w1_write_byte(dev, W1_CMD_RESUME);
out:
	(void)w1_unlock_bus(dev);
	return ret;
}

static int skip_rom(const struct device *dev, const struct w1_slave_config *config)
{
	int ret;
	uint8_t cmd;

	if (!config->overdrive) {
		if (w1_configure(dev, W1_SETTING_SPEED, 0) < 0) {
			return -EIO;
		}
	}

	ret = w1_reset_bus(dev);
	if (ret == 0) {
		return -ENODEV;
	}
	if (ret < 0) {
		return ret;
	}

	cmd = config->overdrive ? W1_CMD_OVERDRIVE_SKIP_ROM : W1_CMD_SKIP_ROM;
	ret = w1_write_byte(dev, cmd);
	if (ret < 0) {
		return ret;
	}

	if (config->overdrive) {
		if (w1_configure(dev, W1_SETTING_SPEED, 1) < 0) {
			return -EIO;
		}
	}

	return 0;
}

int w1_skip_rom(const struct device *dev, const struct w1_slave_config *config)
{
	int ret;

	(void)w1_lock_bus(dev);
	ret = skip_rom(dev, config);
	(void)w1_unlock_bus(dev);
	return ret;
}

static int reset_select(const struct device *dev, const struct w1_slave_config *config)
{
	if (w1_get_slave_count(dev) > 1) {
		return match_rom(dev, config);
	}

	return skip_rom(dev, config);
}

int w1_reset_select(const struct device *dev, const struct w1_slave_config *config)
{
	int ret;

	(void)w1_lock_bus(dev);
	ret = reset_select(dev, config);
	(void)w1_unlock_bus(dev);
	return ret;
}

static int write_read(const struct device *dev, const struct w1_slave_config *config,
		      const uint8_t *write_buf, size_t write_len,
		      uint8_t *read_buf, size_t read_len)
{
	int ret;

	ret = reset_select(dev, config);
	if (ret != 0) {
		return ret;
	}

	ret = w1_write_block(dev, write_buf, write_len);
	if (ret < 0) {
		return ret;
	}

	if (read_buf == NULL && read_len > 0) {
		return -EIO;
	}
	return w1_read_block(dev, read_buf, read_len);
};

int w1_write_read(const struct device *dev, const struct w1_slave_config *config,
		  const uint8_t *write_buf, size_t write_len,
		  uint8_t *read_buf, size_t read_len)
{
	int ret;

	(void)w1_lock_bus(dev);
	ret = write_read(dev, config, write_buf, write_len, read_buf, read_len);
	(void)w1_unlock_bus(dev);
	return ret;
};
