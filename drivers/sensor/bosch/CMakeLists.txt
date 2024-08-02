# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_BMA280 bma280)
add_subdirectory_ifdef(CONFIG_BMA4XX bma4xx)
add_subdirectory_ifdef(CONFIG_BMC150_MAGN bmc150_magn)
add_subdirectory_ifdef(CONFIG_BME280 bme280)
add_subdirectory_ifdef(CONFIG_BME680 bme680)
add_subdirectory_ifdef(CONFIG_BMG160 bmg160)
add_subdirectory_ifdef(CONFIG_BMI08X bmi08x)
add_subdirectory_ifdef(CONFIG_BMI160 bmi160)
add_subdirectory_ifdef(CONFIG_BMI270 bmi270)
add_subdirectory_ifdef(CONFIG_BMI323 bmi323)
add_subdirectory_ifdef(CONFIG_BMM150 bmm150)
add_subdirectory_ifdef(CONFIG_BMP388 bmp388)
add_subdirectory_ifdef(CONFIG_BMP581 bmp581)
# zephyr-keep-sorted-stop
