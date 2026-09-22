# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_ESP32S3_APPCPU)

  zephyr_sources(
    soc_appcpu.c
    )

else()

  zephyr_sources(
    soc.c
    ../common/loader.c
    )

  zephyr_sources_ifdef(CONFIG_SMP esp32s3_smp.c ../common/esp_mp_stall.c)
  zephyr_sources_ifdef(CONFIG_SOC_ENABLE_APPCPU esp32s3_amp.c)

endif()

zephyr_include_directories(.)

zephyr_sources_ifndef(CONFIG_BOOTLOADER_MCUBOOT hw_init.c)

zephyr_library_sources_ifdef(CONFIG_NEWLIB_LIBC newlib_fix.c)

# Power Management
zephyr_library_sources_ifdef(CONFIG_PM ../common/power.c)
zephyr_library_sources_ifdef(CONFIG_POWEROFF ../common/poweroff.c)
