# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bmi08x_accel.c)
zephyr_library_sources(bmi08x_gyro.c)
zephyr_library_sources(bmi08x.c)
zephyr_library_sources_ifdef(CONFIG_BMI08X_ACCEL_TRIGGER bmi08x_accel_trigger.c)
zephyr_library_sources_ifdef(CONFIG_BMI08X_GYRO_TRIGGER bmi08x_gyro_trigger.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API
	bmi08x_bus.c
	bmi08x_gyro_async.c
	bmi08x_gyro_decoder.c
	bmi08x_accel_async.c
	bmi08x_accel_decoder.c
)
zephyr_library_sources_ifdef(CONFIG_BMI08X_GYRO_STREAM bmi08x_gyro_stream.c)
zephyr_library_sources_ifdef(CONFIG_BMI08X_ACCEL_STREAM bmi08x_accel_stream.c)
