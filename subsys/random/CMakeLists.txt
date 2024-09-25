# SPDX-License-Identifier: Apache-2.0

if (CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR OR
    CONFIG_TIMER_RANDOM_GENERATOR OR
    CONFIG_XOSHIRO_RANDOM_GENERATOR)
zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/random/random.h)
zephyr_library()
zephyr_library_sources_ifdef(CONFIG_USERSPACE           random_handlers.c)
endif()

if (CONFIG_TIMER_RANDOM_GENERATOR)
  message(WARNING "
    Warning: CONFIG_TIMER_RANDOM_GENERATOR is not a truly random generator.
    This capability is not secure and it is provided for testing purposes only.
    Use it carefully.")
endif()

zephyr_library_sources_ifdef(CONFIG_TIMER_RANDOM_GENERATOR          random_timer.c)
zephyr_library_sources_ifdef(CONFIG_XOSHIRO_RANDOM_GENERATOR        random_xoshiro128.c)
zephyr_library_sources_ifdef(CONFIG_CTR_DRBG_CSPRNG_GENERATOR       random_ctr_drbg.c)

if (CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR OR CONFIG_HARDWARE_DEVICE_CS_GENERATOR)
zephyr_library_sources(random_entropy_device.c)
endif()

if (CONFIG_CTR_DRBG_CSPRNG_GENERATOR)
zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
endif()
