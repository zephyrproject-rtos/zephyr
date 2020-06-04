# SPDX-License-Identifier: Apache-2.0

if (CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR OR
    CONFIG_TIMER_RANDOM_GENERATOR OR
    CONFIG_XOROSHIRO_RANDOM_GENERATOR)
zephyr_library()
zephyr_library_sources_ifdef(CONFIG_USERSPACE           rand32_handlers.c)
endif()

zephyr_library_sources_ifdef(CONFIG_TIMER_RANDOM_GENERATOR          rand32_timer.c)
zephyr_library_sources_ifdef(CONFIG_XOROSHIRO_RANDOM_GENERATOR      rand32_xoroshiro128.c)
zephyr_library_sources_ifdef(CONFIG_CTR_DRBG_CSPRNG_GENERATOR 		rand32_ctr_drbg.c)

if (CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR OR CONFIG_HARDWARE_DEVICE_CS_GENERATOR)
zephyr_library_sources(rand32_entropy_device.c)
endif()

if (CONFIG_CTR_DRBG_CSPRNG_GENERATOR)
zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
endif()
