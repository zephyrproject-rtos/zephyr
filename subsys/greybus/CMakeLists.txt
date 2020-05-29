# SPDX-License-Identifier: Apache-2.0

zephyr_library()

target_include_directories(subsys__greybus INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library_sources(
	greybus-stubs.c
	greybus-core.c
	greybus-manifest.c
)

zephyr_library_sources_ifdef(CONFIG_GREYBUS_CONTROL        control-gpb.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_AUDIO          audio.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_CAMERA         camera.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_GPIO           gpio.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_DEBUG          greybus-debug.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_HID            hid.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_I2C            i2c.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_LIGHTS         lights.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_LOOPBACK       loopback.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_POWER_SUPPLY   power_supply.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_PWM            pwm-protocol.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_SDIO           sdio.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_SPI            spi.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_UART           uart.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_USB            usb.c)
zephyr_library_sources_ifdef(CONFIG_GREYBUS_VIBRATOR       vibrator.c)
