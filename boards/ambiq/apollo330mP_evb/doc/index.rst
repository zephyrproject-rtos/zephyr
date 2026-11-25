.. zephyr:board:: apollo330mP_evb

Apollo330mP EVB is a board by Ambiq featuring their ultra-low power Apollo330mP SoC.

Hardware
********

- Apollo330mP SoC with up to 250 MHz operating frequency
- ARM® Cortex® M55 core
- Integrated 32 kB Instruction Cache and 32 kB Data Cache
- Up to 2 MB of non-volatile memory2 (NVM) for code/data
- 2 MB of TCM and system RAM for code/data
- Integrated 256 kB Instruction and Data Tightly Coupled Memory (ITCM and DTCM)

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
=========================

.. zephyr:board-supported-runners::

Flashing an application
-----------------------

Connect your device to your host computer using the JLINK USB port.
The sample application :zephyr:code-sample:`hello_world` is used for this example.
Build the Zephyr kernel and application, then flash it to the device:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: apollo330mP_evb
   :goals: flash

.. note::
   ``west flash`` requires `SEGGER J-Link software`_ and `pylink`_ Python module
   to be installed on you host computer.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should be able to see on the corresponding Serial Port
the following message:

.. code-block:: console

   Hello World! apollo330mP_evb

Building other samples
----------------------

The samples and tests below are built for this board as part of the Ambiq
overnight release build set. Paths are relative to the zephyr repository.

* ``samples/basic/blinky_pwm``
* ``samples/basic/button``
* ``samples/basic/threads``
* ``samples/cpp/hello_world``
* ``samples/drivers/adc/adc_dt``
* ``samples/drivers/audio/dmic``
* ``samples/drivers/counter/alarm``
* ``samples/drivers/crc``
* ``samples/drivers/crypto`` (``-- -DCONFIG_NOCACHE_MEMORY=y``)
* ``samples/drivers/eeprom`` (``-- -DSHIELD=ambiq_mikrobus_i2c,mikroe_eeram_33v_click``)
* ``samples/drivers/i2s/output``
* ``samples/drivers/led/pwm``
* ``samples/drivers/mbox_data``
* ``samples/drivers/memc``
* ``samples/drivers/mspi/mspi_timing_scan``
* ``samples/drivers/rtc``
* ``samples/drivers/uart/echo_bot``
* ``samples/drivers/watchdog``
* ``samples/hello_world``
* ``samples/philosophers``
* ``samples/subsys/fs/fs_sample``
* ``samples/subsys/logging/logger``
* ``samples/subsys/usb/mass``
* ``samples/synchronization``
* ``tests/arch/arm/arm_irq_vector_table``
* ``tests/crypto/crypto_aes`` (``-- -DCONFIG_NOCACHE_MEMORY=y``)
* ``tests/crypto/secp256r1`` (``-- -DEXTRA_CONF_FILE=p256-m_raw.conf``)
* ``tests/drivers/adc/adc_api``
* ``tests/drivers/audio/dmic_api``
* ``tests/drivers/counter/counter_basic_api``
* ``tests/drivers/entropy/api``
* ``tests/drivers/flash/common``
* ``tests/drivers/flash/erase_blocks``
* ``tests/drivers/gpio/gpio_basic_api``
* ``tests/drivers/hwinfo/api``
* ``tests/drivers/pwm/pwm_api``
* ``tests/drivers/retained_mem/api``
* ``tests/drivers/rtc/rtc_api``
* ``tests/drivers/watchdog/wdt_basic_api``
* ``tests/kernel/common``
* ``tests/kernel/context``
* ``tests/kernel/fifo/fifo_api``
* ``tests/kernel/mutex/mutex_api``
* ``tests/kernel/queue``
* ``tests/kernel/semaphore/semaphore``
* ``tests/kernel/sleep``
* ``tests/kernel/threads/thread_apis``
* ``tests/kernel/timer/timer_api``
* ``tests/kernel/timer/timer_behavior``
* ``tests/kernel/workq/work``
* ``tests/lib/heap``
* ``tests/subsys/dfu/img_util`` (``-T dfu.image_util``)
* ``tests/subsys/logging/log_api``
* ``tests/subsys/pm/power_mgmt_soc``
* ``tests/subsys/pm/power_residency_time``
* ``tests/subsys/pm/power_states``
* ``tests/subsys/pm/power_wakeup_timer``
* ``tests/subsys/sd/mmc``
* ``tests/subsys/sd/sdio``
* ``tests/subsys/settings/retention``
