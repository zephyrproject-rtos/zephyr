# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_I2C_EEPROM_TARGET eeprom_target.c)
# zephyr-keep-sorted-stop
zephyr_library_sources_ifdef(CONFIG_I2C_HID_TARGET i2c_hid_target.c)
