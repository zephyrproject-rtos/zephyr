#ifndef I2C_MOCK_H
#define I2C_MOCK_H

#include <zephyr/ztest.h>

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/fff.h>

struct i2c_dt_spec {
    struct device *bus;
    uint8_t *burst_buf;
};

DECLARE_FAKE_VALUE_FUNC(int, i2c_reg_write_byte_dt, const struct i2c_dt_spec *, uint8_t, uint8_t);
DECLARE_FAKE_VALUE_FUNC(int, i2c_reg_read_byte_dt, const struct i2c_dt_spec *, uint8_t, uint8_t *);
DECLARE_FAKE_VALUE_FUNC(int, i2c_burst_write_dt, const struct i2c_dt_spec *, uint8_t,
                        const uint8_t *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(int, i2c_reg_update_byte_dt, const struct i2c_dt_spec *, uint8_t, uint8_t,
                        uint8_t);

#endif /* I2C_MOCK_H */
