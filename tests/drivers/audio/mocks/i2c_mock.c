#include <zephyr/ztest.h>

#include <stdint.h>
#include <zephyr/fff.h>

#include "i2c_mock.h"

DEFINE_FAKE_VALUE_FUNC(int, i2c_reg_write_byte_dt, const struct i2c_dt_spec *, uint8_t, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, i2c_reg_read_byte_dt, const struct i2c_dt_spec *, uint8_t, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, i2c_burst_write_dt, const struct i2c_dt_spec *, uint8_t,
                       const uint8_t *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, i2c_reg_update_byte_dt, const struct i2c_dt_spec *, uint8_t, uint8_t,
                       uint8_t);
