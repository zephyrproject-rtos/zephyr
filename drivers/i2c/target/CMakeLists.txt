# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_I2C_EEPROM_TARGET eeprom_target.c)
zephyr_library_sources_ifdef(CONFIG_I2C_TARGET_REGSET_LIB regset_target_lib.c)
zephyr_library_sources_ifdef(CONFIG_I2C_TMP103_TARGET tmp103_target.c)
# zephyr-keep-sorted-stop
