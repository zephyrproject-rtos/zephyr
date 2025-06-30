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

Whether it’s the Real Time Clock (RTC) IC, or a System-on-a-Chip (SoC), Ambiq® is committed to enabling the
lowest power consumption with the highest computing performance possible for our customers to make the most
innovative battery-power endpoint devices for their end-users. `Ambiq Products`_

Status
***************

As of now, Ambiq provides zephyr support for a set of peripherals/drivers:

+--------+--------------------+--------------------+------------------------------------------+
| Driver |     Apollo510      |   Stable codes at  |              Sample                      |
+========+====================+====================+==========================================+
|   ADC  |         -          |        main        | samples\\drivers\\adc\\adc\_dt           |
+--------+--------------------+--------------------+------------------------------------------+
| AUDADC |    comming soon    |                    |                                          |
+--------+--------------------+--------------------+------------------------------------------+
| COUNTER|         -          |        main        | samples\\drivers\\counter\\alarm         |
+--------+--------------------+--------------------+------------------------------------------+
| CRYPTO |    comming soon    |                    |                                          |
+--------+--------------------+--------------------+------------------------------------------+
| DISPLAY|    comming soon    |                    |                                          |
+--------+--------------------+--------------------+------------------------------------------+
| FLASH  |         -          |        main        |  tests\\drivers\\flash\_api              |
+--------+--------------------+--------------------+------------------------------------------+
| HWINFO |         -          |        main        |  tests\\drivers\\hwinfo\\api             |
+--------+--------------------+--------------------+------------------------------------------+
|   I2C  |         -          |        main        |  samples\\drivers\\eeprom                |
+--------+--------------------+--------------------+------------------------------------------+
|   I2S  |         -          |        main        |  samples\\drivers\\i2s\\dmic\_i2s        |
+--------+--------------------+--------------------+------------------------------------------+
|MIPI_DSI|    comming soon    |                    |                                          |
+--------+--------------------+--------------------+------------------------------------------+
|  MSPI  |         -          |        main        |   samples\\drivers\\mspi\\mspi\_flash    |
+--------+--------------------+--------------------+------------------------------------------+
|   PDM  |         -          |        main        |    samples\\drivers\\audio\\dmic         |
+--------+--------------------+--------------------+------------------------------------------+
|   PM   |         -          |        main        |    samples\\subsys\\pm\\device\_pm       |
+--------+--------------------+--------------------+------------------------------------------+
|   PWM  |         -          |    apollo510-dev   |    samples\\drivers\\led\\pwm            |
+--------+--------------------+--------------------+------------------------------------------+
|   RTC  |         -          |        main        |    samples\\drivers\\rtc                 |
+--------+--------------------+--------------------+------------------------------------------+
|  SDHC  |         -          |    apollo510-dev   |samples\\drivers\\subsys\\fs\\fs\_sample  |
+--------+--------------------+--------------------+------------------------------------------+
|   SPI  |         -          |        main        |samples\\boards\\ambiq\\spi\_serial\_flash|
+--------+--------------------+--------------------+------------------------------------------+
|  TIMER |         -          |        main        |    samples\\philosophers                 |
+--------+--------------------+--------------------+------------------------------------------+
|  UART  |         -          |        main        |   samples\\drivers\\uart\\echo\_bot      |
+--------+--------------------+--------------------+------------------------------------------+
|   USB  |         -          |        main        |  samples\\drivers\\subsys\\usb\\mass     |
+--------+--------------------+--------------------+------------------------------------------+

And also there are supports for some third-party libs:

+----------+------------------+--------------------+------------------------------------------+
|    Lib   |     Apollo510    |   Stable codes at  |              Sample                      |
+==========+==================+====================+==========================================+
| coremark |         -        |    ambiq-coremark  | samples\\benchmarks\\coremark            |
+----------+------------------+--------------------+------------------------------------------+
|   fatfs  |         -        |        main        | samples\\drivers\\subsys\\fs\\fs\_sample |
+----------+------------------+--------------------+------------------------------------------+
|  mbedtls |    comming soon  |                    |                                          |
+----------+------------------+--------------------+------------------------------------------+
|   lvgl   |    comming soon  |                    |                                          |
+----------+------------------+--------------------+------------------------------------------+


Together with generic support for ARM Cortex-M peripherals like cache, interrupt controller, etc.

And please note that apollo510_eb board is only available at apollo510-dev branch, some samples
rely on the devices on the EB board, for example I2C and SPI.

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

Execute ``git checkout apollo510-dev`` to get the latest apollo510 development branch.


Get to Know Ambiq Components
----------------------------

.. code-block:: text

  zephyr/
  │
  ├── boards/
  │   └── ambiq/
  │       └── apollo510_evb
  ├── drivers/
  │   ├── adc/
  │   │   └── adc_ambiq.c
  │   ├── audio/
  │   │   └── dmic_ambiq_pdm.c
  │   ├── bluetooth/
  │   │   └── hci/
  │   │       ├── apollox_blue.c
  │   │       └── hci_ambiq.c
  │   ├── clock_control/
  │   │   └── clock_control_ambiq.c
  │   ├── counter/
  │   │   └── counter_ambiq_timer.c
  │   ├── flash/
  │   │   └── flash_ambiq.c
  │   ├── gpio/
  │   │   └── gpio_ambiq.c
  │   ├── hwinfo/
  │   │   └── hwinfo_ambiq.c
  │   ├── i2c/
  │   │   └── i2c_ambiq.c
  │   ├── i2s/
  │   │   └── i2s_ambiq.c
  │   ├── mspi/
  │   │   └── mspi_ambiq_ap5.c
  │   ├── pinctrl/
  │   │   └── pinctrl_ambiq.c
  │   ├── pwm/
  │   │   └── pwm_ambiq_timer.c
  │   ├── rtc/
  │   │   └── rtc_ambiq.c
  │   ├── sdhc/
  │   │   └── sdhc_ambiq.c
  │   ├── serial/
  │   │   └── uart_ambiq.c
  │   ├── spi/
  │   │   ├── spi_ambiq_spic.c
  │   │   └── spi_ambiq_spid.c
  │   ├── timer/
  │   │   └── ambiq_stimer.c
  │   ├── usb/
  │   │   └── udc/
  │   │       └── udc_ambiq.c
  │   ├── watchdog/
  │   │   └── wdt_ambiq.c
  ├── dts/
  │   └── arm/
  │       └── ambiq/
  │           └── ambiq_apollo510.dtsi
  └── soc/
      └── ambiq/
          └── apollo5x


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
.. _Source Code Repository: https://github.com/AmbiqMicro/ambiqhal_ambiq
