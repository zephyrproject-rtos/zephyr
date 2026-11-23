/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FAKE_I2C_DEV_BOTTOM_H
#define FAKE_I2C_DEV_BOTTOM_H

/* Paths of the fake adapters, as in app.overlay; any other path does not exist */
#define FAKE_I2C_DEV_PATH       "/dev/i2c-fake"
#define FAKE_I2C_DEV_SMBUS_PATH "/dev/i2c-fake-smbus"

/* Address of the register file of the fake adapter, any other address NACKs */
#define FAKE_I2C_DEV_ADDR 0x50

/* Forget the transfers recorded so far */
void fake_i2c_dev_reset(void);

/* Number of I2C_RDWR transfers since the last reset */
int fake_i2c_dev_transfers(void);

/* Number of messages of a recorded transfer */
int fake_i2c_dev_msgs(int transfer);

/* Length of a message of a recorded transfer */
int fake_i2c_dev_msg_len(int transfer, int msg);

/* 1 when a message of a recorded transfer is a read, 0 otherwise */
int fake_i2c_dev_msg_read(int transfer, int msg);

/* 1 when a message of a recorded transfer uses a 10-bit address, 0 otherwise */
int fake_i2c_dev_msg_ten(int transfer, int msg);

#endif /* FAKE_I2C_DEV_BOTTOM_H */
