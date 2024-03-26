# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bmi08x_accel.c)
zephyr_library_sources(bmi08x_gyro.c)
zephyr_library_sources(bmi08x.c)
zephyr_library_sources_ifdef(CONFIG_BMI08X_ACCEL_TRIGGER bmi08x_accel_trigger.c)
zephyr_library_sources_ifdef(CONFIG_BMI08X_GYRO_TRIGGER bmi08x_gyro_trigger.c)
