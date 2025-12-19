.. raw:: html

   <a href="https://www.zephyrproject.org">
     <p align="center">
       <picture>
         <img src="doc/_static/images/Zephyr-support-for-Ambiq.svg">
       </picture>
     </p>
   </a>

   <a href="https://bestpractices.coreinfrastructure.org/projects/74"><img src="https://bestpractices.coreinfrastructure.org/projects/74/badge"></a>
   <a href="https://scorecard.dev/viewer/?uri=github.com/zephyrproject-rtos/zephyr"><img src="https://api.securityscorecards.dev/projects/github.com/zephyrproject-rtos/zephyr/badge"></a>
   <a href="https://github.com/zephyrproject-rtos/zephyr/actions/workflows/twister.yaml?query=branch%3Amain"><img src="https://github.com/zephyrproject-rtos/zephyr/actions/workflows/twister.yaml/badge.svg?event=push"></a>

Ambiq® is an Austin-based SoC vendor who at the forefront of enabling ambient intelligence on billions of
devices with the unique SPOT platform and extreme-low power semiconductor solutions.

Whether it's the Real Time Clock (RTC) IC, or a System-on-a-Chip (SoC), Ambiq® is committed to enabling the
lowest power consumption with the highest computing performance possible for our customers to make the most
innovative battery-power endpoint devices for their end-users. `Ambiq Products`_

Status
******

As of now, Ambiq provides zephyr support for a set of peripherals/drivers:

+--------+----------------+--------------------+-------------------------------------------+------------------+
| Driver |   Apollo510    |   Stable codes at  |              Sample                       |       Board      |
+========+================+====================+===========================================+==================+
|   ADC  |       -        |    ambiq-stable    | samples\\drivers\\adc\\adc\_dt            |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
| AUDADC |       -        |    ambiq-stable    | samples\\drivers\\audio\\amic             |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|   BLE  |       -        |    ambiq-stable    | samples\\bluetooth                        | blue boards only |
+--------+----------------+--------------------+-------------------------------------------+------------------+
| COUNTER|                |    ambiq-stable    | samples\\drivers\\counter\\alarm          |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
| CRYPTO |  coming soon   |                    |                                           |                  |
+--------+----------------+--------------------+-------------------------------------------+------------------+
| DISPLAY|       -        |    ambiq-stable    |  samples\\drivers\\display                |    ap510_disp    |
+--------+----------------+--------------------+-------------------------------------------+------------------+
| FLASH  |       -        |    ambiq-stable    |  samples\\subsys\\mgmt\\mcumgr\\smp_svr   |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
| HWINFO |       -        |    ambiq-stable    |  tests\\drivers\\hwinfo\\api              |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|   I2C  |       -        |    ambiq-stable    |  samples\\drivers\\eeprom                 |apollo510_eb only |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|   I2S  |       -        |    ambiq-stable    |  samples\\drivers\\i2s\\dmic\_i2s         |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|  INPUT |       -        |    ambiq-stable    |samples\\subsys\\input\\draw\_touch\_events|    ap510_disp    |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|  JDI   |       -        |    ambiq-stable    |  samples\\drivers\\display                |  ap510_jdi_disp  |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|MIPI_DBI|       -        |    ambiq-stable    |  samples\\drivers\\display                |display_8080_card |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|MIPI_DSI|       -        |    ambiq-stable    |  samples\\drivers\\display                |    ap510_disp    |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|  MSPI  |       -        |    ambiq-stable    |   samples\\drivers\\mspi\\mspi\_flash     |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|   PDM  |       -        |    ambiq-stable    |    samples\\drivers\\audio\\dmic          |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|   PM   |       -        |    ambiq-stable    |    tests\\subsys\\pm\\power_wakeup_timer  |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|   PWM  |       -        |    ambiq-stable    |    samples\\drivers\\led\\pwm             |  evb boards only |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|   RTC  |       -        |    ambiq-stable    |    samples\\drivers\\rtc                  |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|  SDHC  |       -        |    ambiq-stable    |         tests\\subsys\\sd\\mmc            |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|   SPI  |       -        |    ambiq-stable    |samples\\boards\\ambiq\\spi\_serial\_flash |apollo510_eb only |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|  TIMER |       -        |    ambiq-stable    |    samples\\philosophers                  |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|  TRNG  |       -        |    ambiq-stable    |  tests\\drivers\\entropy\\api             |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|  UART  |       -        |    ambiq-stable    |   samples\\drivers\\uart\\echo\_bot       |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|   USB  |       -        |    ambiq-stable    |  samples\\drivers\\subsys\\usb\\mass      |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|   WDT  |       -        |    ambiq-stable    |  samples\\subsys\\task_wdt                |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+

And also there are supports for some third-party libs:

+--------+----------------+--------------------+-------------------------------------------+------------------+
|   Lib  |    Apollo510   |   Stable codes at  |              Sample                       |       Board      |
+========+================+====================+===========================================+==================+
|coremark|       -        |    ambiq-stable    | samples\\benchmarks\\coremark             |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|  fatfs |       -        |    ambiq-stable    | samples\\subsys\\fs\\fs\_sample           |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
| mbedtls| software only  |    ambiq-stable    | tests\\crypto\\mbedtls                    |        All       |
+--------+----------------+--------------------+-------------------------------------------+------------------+
|  lvgl  |       -        |    ambiq-stable    | samples\\modules\\lvgl\\demos             |    ap510_disp    |
+--------+----------------+--------------------+-------------------------------------------+------------------+


Together with generic support for ARM Cortex-M peripherals like cache, interrupt controller, etc.


.. below included in doc/introduction/introduction.rst


Getting Started
***************

Welcome to Ambiq Zephyr! See the `Introduction to Zephyr`_ for a high-level overview,
and the documentation's `Getting Started Guide`_ to start developing.

Check `Ambiq SoC`_ for Ambiq Apollo SoCs documents.


Setup Development Environment
-----------------------------

Follow `Getting Started Guide`_ to install denpendencies and use west tool to get the Zephyr source code to local.
Install Zephyr SDK and check whether the environment variables are set properly.


Upstream Repository Synchronization
-----------------------------------

Execute ``git remote -v`` to check if upstream has been configured.

If not, execute ``git remote add upstream https://github.com/AmbiqMicro/ambiqzephyr`` to configure the ambiqzephyr base to your upstream repository.

Execute ``git remote -v`` again to check if it configures successfully.

Execute ``git fetch upstream`` to fetch the upstream repository.

Execute ``git checkout ambiq-stable`` to get the latest ambiq soc development branch.


Get to Know Ambiq Components
----------------------------

.. code-block:: text

  zephyr/
  │
  ├── boards/
  │   ├── ambiq/
  │   └── shields/
  │       ├── ap4_evb_disp_shield_rev2
  │       ├── ap510_disp
  │       ├── ap510_jdi_disp
  │       └── apollo5_eb_display_8080_card
  ├── drivers/
  │   ├── adc/
  │   │   └── adc_ambiq.c
  │   ├── audio/
  │   │   ├── amic_ambiq_audadc.c
  │   │   └── dmic_ambiq_pdm.c
  │   ├── bluetooth/
  │   │   └── hci/
  │   │       ├── apollox_blue.c
  │   │       └── hci_ambiq.c
  │   ├── clock_control/
  │   │   └── clock_control_ambiq.c
  │   ├── counter/
  │   │   └── counter_ambiq_timer.c
  │   ├── display/
  │   │   ├── display_co5300.c
  │   │   └── display_ls014b7dd01.c
  │   ├── entropy/
  │   │   └── entropy_ambiq_puf_trng.c
  │   ├── flash/
  │   │   └── flash_ambiq.c
  │   ├── gpio/
  │   │   └── gpio_ambiq.c
  │   ├── hwinfo/
  │   │   └── hwinfo_ambiq.c
  │   ├── i2c/
  │   │   ├── i2c_ambiq.c
  │   │   └── i2c_ambiq_ios.c
  │   ├── i2s/
  │   │   └── i2s_ambiq.c
  │   ├── jdi/
  │   │   └── jdi_ambiq.c
  │   ├── mipi_dbi/
  │   │   └── mipi_dbi_ambiq.c
  │   ├── mipi_dsi/
  │   │   └── dsi_ambiq.c
  │   ├── mspi/
  │   │   ├── mspi_ambiq_ap3.c
  │   │   ├── mspi_ambiq_ap4.c
  │   │   ├── mspi_ambiq_ap5.c
  │   │   └── mspi_ambiq_timing_scan.c
  │   ├── pinctrl/
  │   │   └── pinctrl_ambiq.c
  │   ├── pwm/
  │   │   ├── pwm_ambiq_ctimer.c
  │   │   └── pwm_ambiq_timer.c
  │   ├── rtc/
  │   │   └── rtc_ambiq.c
  │   ├── sdhc/
  │   │   └── sdhc_ambiq.c
  │   ├── serial/
  │   │   └── uart_ambiq.c
  │   ├── spi/
  │   │   ├── spi_ambiq_bleif.c
  │   │   ├── spi_ambiq_dcif.c
  │   │   ├── spi_ambiq_spic.c
  │   │   └── spi_ambiq_spid.c
  │   ├── timer/
  │   │   └── ambiq_stimer.c
  │   ├── usb/
  │   │   └── udc/
  │   │       └── udc_ambiq.c
  │   └── watchdog/
  │       └── wdt_ambiq.c
  ├── dts/
  │   └── arm/
  │       └── ambiq/
  ├── modules/
  │   └── hal_ambiq
  └── soc/
      └── ambiq/


Build and Flash the Samples
---------------------------

Make sure you have already installed proper version of JLINK which supports corresponding ambiq SoC, and
added the path of JLINK.exe (e.g. C:\Program Files\SEGGER\JLink) to the environment variables.

Go the Zephyr root path, execute ``west build -b <your-board-name> <samples> -p always`` to build the samples for your board.
For example, build zephyr/samples/hello_world for apollo510_evb: ``west build -b apollo510_evb ./samples/hello_world -p always``.

Execute ``west flash`` to flash the binary to the EVB if the zephyr.bin has been generated by west build.

In default we use UART COM for console, and the default baudrate is 115200, so after west flash, open the serial terminal and set proper baudrate for the UART COM of plugged EVB.

You should be able to see the logs in the serial terminal.

``*** Booting Zephyr OS build v4.1.0-7246-gad4c3e3e9afe ***``

``Hello World! apollo510_evb/apollo510``

For those samples that require additional hardware, such as the ap510_disp shield, you need to set the shield option when building. For example:

``west build -b apollo510_evb --shield ap510_disp ./samples/drivers/display -p always``

For Bluetooth samples, you need to program the BLE Controller firmware via JLINK once before running samples. The programming script and binary locate in ambiq SDK ambiqsuite.
Please get the SDK from Ambiq Content Portal.

For MSPI samples, please refer to How_to_Run_Zephyr_MSPI_Samples_and_Tests.rst

For USB samples, please refer to How_to_Run_Zephyr_USB_Samples.rst

For MCU_Boot samples, please refer to How_to_Run_MCUBoot_Samples_and_Tests.rst


Power Management Instructions
-----------------------------

To achieve lowest power consumption, customer needs to follow the following process for inspection and configuration optimization:

1. According to the actual usage of memory, adjust the memory configurations (e.g., SRAM, DTCM, NVM) in soc_early_init_hook;

2. Make sure ``CONFIG_PM=y``, ``CONFIG_PM_DEVICE=y``.

3. There are 2 ways for drivers PM to operate: the first one is to keep ``CONFIG_PM_DEVICE_RUNTIME=y`` and ``CONFIG_PM_DEVICE_RUNTIME_DEFAULT_ENABLE=y``, so the drivers that have runtime PM ability
   should be power-managed automatically; and the other one is to keep ``CONFIG_PM_DEVICE_SYSTEM_MANAGED=y`` and drivers will be suspended automatically before deep sleep.
   Note: These two methods can coexist. Devices with runtime PM enabled will be managed by runtime PM, while other devices will be managed by system-managed PM if enabled.

4. If choose the first way (runtime PM) in step 3, in application code, call pm_device_runtime_put() / pm_device_runtime_get() around peripherals that should power-cycle on demand to ensure their usage
   count returns to zero after use.

5. In order to run CI without print log error, we keep the console UART always on by default. Users need to set ``CONFIG_PM_DEVICE_RUNTIME_DISABLE_CONSOLE=n`` in their own project to resume PM control
   on it.

6. Configure CPU performance mode to low power mode when high performance is not required. This can be done via devicetree (set ``ambiq,perf-mode = <AMBIQ_POWER_MODE_LOW_POWER>``) or by calling
   ``apollo5x_set_performance_mode(AMBIQ_POWER_MODE_LOW_POWER)`` in application code.

7. Disable unused peripheral power domains in soc_early_init_hook (e.g., DEBUG, CRYPTO, OTP) if they are not needed in your application.

8. Disable unused peripherals in devicetree by setting ``status = "disabled"`` for nodes that are not used in your application.

9. Reduce log level in production builds by setting appropriate ``CONFIG_LOG_*`` options to minimize logging overhead and power consumption.

Check `Zephyr Power Management`_ for more detailed information.

.. start_include_here

Community Support
*****************

Community support is provided via mailing lists and Discord; see the Resources
below for details.

.. _project-resources:

Resources
*********

Here's a quick summary of resources to help you find your way around:

Getting Started
---------------

  | 📖 `Zephyr Documentation`_
  | 🚀 `Getting Started Guide`_
  | 🙋🏽 `Tips when asking for help`_
  | 💻 `Code samples`_

Code and Development
--------------------

  | 🌐 `Source Code Repository`_
  | 🌐 `Ambiq HAL Repository`_
  | 📦 `Releases`_
  | 🤝 `Contribution Guide`_

Community and Support
---------------------

  | 💬 `Discord Server`_ for real-time community discussions
  | 📧 `User mailing list (users@lists.zephyrproject.org)`_
  | 📧 `Developer mailing list (devel@lists.zephyrproject.org)`_
  | 📬 `Other project mailing lists`_
  | 📚 `Project Wiki`_

Issue Tracking and Security
---------------------------

  | 🐛 `GitHub Issues`_
  | 🔒 `Security documentation`_
  | 🛡️ `Security Advisories Repository`_
  | ⚠️ Report security vulnerabilities at vulnerabilities@zephyrproject.org

Additional Resources
--------------------
  | 🌐 `Zephyr Project Website`_
  | 📺 `Zephyr Tech Talks`_

.. _Zephyr Project Website: https://www.zephyrproject.org
.. _Discord Server: https://chat.zephyrproject.org
.. _Zephyr Documentation: https://docs.zephyrproject.org
.. _Introduction to Zephyr: https://docs.zephyrproject.org/latest/introduction/index.html
.. _Getting Started Guide: https://docs.zephyrproject.org/latest/develop/getting_started/index.html
.. _Contribution Guide: https://docs.zephyrproject.org/latest/contribute/index.html
.. _Source Code Repository: https://github.com/AmbiqMicro/ambiqzephyr
.. _GitHub Issues: https://github.com/AmbiqMicro/ambiqzephyr/issues
.. _Releases: https://github.com/zephyrproject-rtos/zephyr/releases
.. _Project Wiki: https://github.com/zephyrproject-rtos/zephyr/wiki
.. _User mailing list (users@lists.zephyrproject.org): https://lists.zephyrproject.org/g/users
.. _Developer mailing list (devel@lists.zephyrproject.org): https://lists.zephyrproject.org/g/devel
.. _Other project mailing lists: https://lists.zephyrproject.org/g/main/subgroups
.. _Code samples: https://docs.zephyrproject.org/latest/samples/index.html
.. _Security documentation: https://docs.zephyrproject.org/latest/security/index.html
.. _Security Advisories Repository: https://github.com/zephyrproject-rtos/zephyr/security
.. _Tips when asking for help: https://docs.zephyrproject.org/latest/develop/getting_started/index.html#asking-for-help
.. _Zephyr Tech Talks: https://www.zephyrproject.org/tech-talks
.. _Ambiq SoC: https://contentportal.ambiq.com/soc
.. _Ambiq Products: https://ambiq.com/products/
.. _Ambiq Content Portal: https://contentportal.ambiq.com/
.. _Ambiq HAL Repository: https://github.com/AmbiqMicro/ambiqhal_ambiq
.. _Zephyr Power Management: https://docs.zephyrproject.org/latest/services/pm/index.html
