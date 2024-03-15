.. _emulators:

Zephyr's device emulators/simulators
####################################

Overview
========

Zephyr includes in its codebase a set of device emulators/simulators.
With this we refer to SW components which are built together with the embedded SW
and present themselves as devices of a given class to the rest of the system.

These device emulators/simulators can be built for any target which has sufficient RAM and flash,
even if some may have extra functionality which is only available in some targets.

.. note::

   | Zephyr also includes and uses many other types of simulators/emulators, including CPU and
     platform simulators, radio simulators, and several build targets which allow running the
     embedded code in the development host.
   | Some of Zephyr communication controllers/drivers include also either loopback modes or loopback
     devices.
   | This page does not cover any of these.

.. note::
   Drivers which are specific to some platform, like for example the
   :ref:`native_sim specific drivers <native_sim_peripherals>` which
   emulate a peripheral class by connecting to host APIs are not covered by this page.


Available Emulators
===================

**ADC emulator**
  * A fake driver which pretends to be actual ADC, and can be used for testing higher-level API
    for ADC devices.
  * Main Kconfig option: :kconfig:option:`CONFIG_ADC_EMUL`
  * DT binding: :dtcompatible:`zephyr,adc-emul`

**DMA emulator**
  * Emulated DMA controller
  * Main Kconfig option: :kconfig:option:`CONFIG_DMA_EMUL`
  * DT binding: :dtcompatible:`zephyr,dma-emul`

**EEPROM emulator**
  * Emulate an EEPROM on a flash partition
  * Main Kconfig option: :kconfig:option:`CONFIG_EEPROM_EMULATOR`
  * DT binding: :dtcompatible:`zephyr,emu-eeprom`

.. _emul_eeprom_simu_brief:

**EEPROM simulator**
  * Emulate an EEPROM on RAM
  * Main Kconfig option: :kconfig:option:`CONFIG_EEPROM_SIMULATOR`
  * DT binding: :dtcompatible:`zephyr,sim-eeprom`
  * Note: For :ref:`native targets <native_sim>` it is also possible to keep the content
    as a file on the host filesystem.

**External bus and bus connected peripheral emulators**
  * :ref:`Documentation <bus_emul>`
  * Allow emulating external buses like I2C or SPI and peripherals connected to them.

.. _emul_flash_simu_brief:

**Flash simulator**
  * Emulate a flash on RAM
  * Main Kconfig option: :kconfig:option:`CONFIG_FLASH_SIMULATOR`
  * DT binding: :dtcompatible:`zephyr,sim-flash`
  * Note: For native targets it is also possible to keep the content as a file on the host
    filesystem. Check :ref:`the native_sim flash simulator section <nsim_per_flash_simu>`.

**GPIO emulator**
  * Emulated GPIO controllers which can be driven from SW
  * Main Kconfig option: :kconfig:option:`CONFIG_GPIO_EMUL`
  * DT binding: :dtcompatible:`zephyr,gpio-emul`

**I2C emulator**
  * Emulated I2C bus. See :ref:`bus emulators <bus_emul>`.
  * Main Kconfig option: :kconfig:option:`CONFIG_I2C_EMUL`
  * DT binding: :dtcompatible:`zephyr,i2c-emul-controller`

**RTC emulator**
  * Emulated RTC peripheral. See :ref:`RTC emulated device section <rtc_api_emul_dev>`
  * Main Kconfig option: :kconfig:option:`CONFIG_RTC_EMUL`
  * DT binding: :dtcompatible:`zephyr,rtc-emul`

**SPI emulator**
  * Emulated SPI bus. See :ref:`bus emulators <bus_emul>`.
  * Main Kconfig option: :kconfig:option:`CONFIG_SPI_EMUL`
  * DT binding: :dtcompatible:`zephyr,spi-emul-controller`

**UART emulator**
  * Emulated UART bus. See :ref:`bus emulators <bus_emul>`.
  * Main Kconfig option: :kconfig:option:`CONFIG_UART_EMUL`
  * DT binding: :dtcompatible:`zephyr,uart-emul`
