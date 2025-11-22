# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(lsm6dsr.c)
zephyr_library_sources(lsm6dsr_spi.c)
zephyr_library_sources(lsm6dsr_i2c.c)
zephyr_library_sources_ifdef(CONFIG_LSM6DSR_TRIGGER    lsm6dsr_trigger.c)
