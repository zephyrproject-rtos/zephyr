# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/eeprom.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE eeprom_handlers.c)
zephyr_library_sources_ifdef(CONFIG_EEPROM_SHELL eeprom_shell.c)

zephyr_library_sources_ifdef(CONFIG_EEPROM_AT2X eeprom_at2x.c)
zephyr_library_sources_ifdef(CONFIG_EEPROM_LPC11U6X eeprom_lpc11u6x.c)
zephyr_library_sources_ifdef(CONFIG_EEPROM_STM32 eeprom_stm32.c)
if(CONFIG_EEPROM_SIMULATOR)
  zephyr_library_sources(eeprom_simulator.c)
  if(CONFIG_NATIVE_LIBRARY)
    target_sources(native_simulator INTERFACE eeprom_simulator_native.c)
  elseif(CONFIG_ARCH_POSIX)
    zephyr_library_sources(eeprom_simulator_native.c)
  endif()
endif()
zephyr_library_sources_ifdef(CONFIG_EEPROM_EMULATOR eeprom_emulator.c)
zephyr_library_sources_ifdef(CONFIG_EEPROM_TMP11X eeprom_tmp11x.c)
zephyr_library_sources_ifdef(CONFIG_EEPROM_XEC eeprom_mchp_xec.c)
zephyr_library_sources_ifdef(CONFIG_EEPROM_FAKE eeprom_fake.c)

zephyr_library_sources_ifdef(CONFIG_EEPROM_AT2X_EMUL eeprom_at2x_emul.c)

zephyr_library_sources_ifdef(CONFIG_EEPROM_MB85RCXX eeprom_mb85rcxx.c)

zephyr_library_sources_ifdef(CONFIG_EEPROM_MB85RSXX eeprom_mb85rsxx.c)
