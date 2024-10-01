.. _nrf7002dk_nrf5340:

nRF7002 DK
##########

Overview
********

The nRF7002 DK (PCA10143) is a single-board development kit for evaluation and development on
the nRF7002, a Wi-Fi companion IC to Nordic Semiconductor's nRF5340 System-on-Chip (SoC) host
processor. It is certified for the Wi-Fi Alliance® `Wi-Fi Certification program`_ in the
Connectivity, Security, and Optimization categories. See `UG Wi-Fi certification`_ for detailed
information.

The nRF7002 is an IEEE 802.11ax (Wi-Fi 6) compliant solution that implements the Wi-Fi physical
layer and Medium Access Control (MAC) layer protocols. It implements the nRF Wi-Fi driver
software on the nRF5340 host processor communicating over the QSPI bus.

The nRF5340 host is a dual-core SoC based on the Arm® Cortex®-M33 architecture.
It has the following features:

* A full-featured Arm Cortex-M33F core with DSP instructions, FPU, and Armv8-M Security Extension,
  running at up to 128 MHz, referred to as the application core.
* A secondary Arm Cortex-M33 core, with a reduced feature set, running at a fixed 64 MHz,
  referred to as the network core.

The ``nrf7002dk/nrf5340/cpuapp`` board target provides support for the application core on the
nRF5340 SoC. The ``nrf7002dk/nrf5340/cpunet`` board target provides support for the network
core on the nRF5340 SoC.

.. figure:: img/nrf7002dk.jpg
   :align: center
   :alt: nRF7002 DK

   nRF7002 DK (Credit: Nordic Semiconductor)

More information about the board can be found at the
`nRF7002 DK website`_.
The `nRF7002 DK Product Specification`_
contains the processor's information and the datasheet.

Hardware
========

* nRF7002 DK:
  The nRF7002 DK has two external oscillators.

  * The frequency of the slow clock is 32.768 kHz.
  * The frequency of the main clock is 32 MHz.

* Micro-USB 2.0 cable

Supported features
------------------

The ``nrf7002dk/nrf5340/cpuapp`` board configuration supports the following hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| ADC       | on-chip    | adc                  |
+-----------+------------+----------------------+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| RTT       | Segger     | console              |
+-----------+------------+----------------------+
| RADIO     | nrf7002    | Wi-Fi 6 (802.11ax)   |
+-----------+------------+----------------------+
| QSPI      | on-chip    | qspi                 |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| SPU       | on-chip    | system protection    |
+-----------+------------+----------------------+
| UARTE     | on-chip    | serial               |
+-----------+------------+----------------------+
| USB       | on-chip    | usb                  |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

The ``nrf7002dk/nrf5340/cpunet`` board configuration supports the following hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| RADIO     | on-chip    | Bluetooth,           |
|           |            | ieee802154           |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| RTT       | Segger     | console              |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UARTE     | on-chip    | serial               |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Other hardware features have not been enabled yet for this board.
See `nRF7002 DK Product Specification`_
for a complete list of nRF7002 DK board hardware features.

Connections and IOs
-------------------

The connections and IOs supported by the development kit are listed in this section.

LED
^^^

* **LED 1** (green) = **P1.06**
* **LED 2** (green) = **P1.07**

Push buttons
^^^^^^^^^^^^

* **Button 1** = **SW1** = **P1.08**
* **Button 2** = **SW2** = **P1.09**
* **BOOT** = **SW5** = boot/reset

Wi-Fi control
^^^^^^^^^^^^^

* BUCKEN = **P0.12**
* IOVDD CONTROL = **P0.31**
* HOST IRQ = **P0.23**
* COEX_REQ = **P0.28**
* COEX_STATUS0 = **P0.30**
* COEX_STATUS1 = **P0.29**
* COEX_GRANT = **P0.24**

Security components
-------------------

The following security components are available:

* Implementation Defined Attribution Unit (`IDAU`_) on the application core.

  The IDAU is implemented with the System Protection Unit and is used to define
  secure and non-secure memory maps. By default, the entire memory space
  (Flash, SRAM, and peripheral address space) is defined to be secure-accessible only.

* Secure boot.

Programming and Debugging
*************************

The nRF5340 application core supports the Armv8-M Security Extension.
Applications built for the ``nrf7002dk/nrf5340/cpuapp`` board boot by default in the
secure state.

The nRF5340 network core does not support the Armv8-M Security Extension.
nRF5340 IDAU can configure bus accesses by the nRF5340 network core to have the secure
attribute set. This allows to build and run secure-only applications on the nRF5340 SoC.

Building Secure/Non-Secure Zephyr applications with Arm |reg| TrustZone |reg|
=============================================================================

Applications on the nRF5340 may contain a Secure and a Non-Secure firmware
image for the application core. The Secure image can be built using either
Zephyr or `Trusted Firmware M`_ (TF-M). Non-Secure firmware
images are always built using Zephyr. The two alternatives are described below.

.. note::

   By default, SPE for the nRF5340 application core is built using TF-M.

Building the Secure firmware with TF-M
--------------------------------------

The process to build the Secure firmware image using TF-M and the Non-Secure
firmware image using Zephyr requires the following steps:

1. Build the Non-Secure Zephyr application
   for the application core using ``-DBOARD=nrf7002dk/nrf5340/cpuapp/ns``.
   To invoke the building of TF-M the Zephyr build system requires the
   Kconfig option ``BUILD_WITH_TFM`` to be enabled, which is done by
   default when building Zephyr as a Non-Secure application.
   The Zephyr build system will perform the following steps automatically:

      * Build the Non-Secure firmware image as a regular Zephyr application
      * Build a TF-M (secure) firmware image
      * Merge the output image binaries together
      * Optionally build a bootloader image (MCUboot)

.. note::

   Depending on the TF-M configuration, an application DTS overlay may be
   required, to adjust the Non-Secure image Flash and SRAM starting address
   and sizes.

2. Build the application firmware for the network core using
   ``-DBOARD=nrf7002dk/nrf5340/cpunet``.

Building the Secure firmware using Zephyr
-----------------------------------------

The process to build the Secure and the Non-Secure firmware images
using Zephyr requires the following steps:

1. Build the Secure Zephyr application for the application core
   using ``-DBOARD=nrf7002dk/nrf5340/cpuapp`` and
   ``CONFIG_TRUSTED_EXECUTION_SECURE=y`` and ``CONFIG_BUILD_WITH_TFM=n``
   in the application project configuration file.
2. Build the Non-Secure Zephyr application for the application core
   using ``-DBOARD=nrf7002dk/nrf5340/cpuapp/ns``.
3. Merge the two binaries together.
4. Build the application firmware for the network core using
   ``-DBOARD=nrf7002dk/nrf5340/cpunet``.

When building a Secure/Non-Secure application for the nRF5340 application core,
the Secure application will have to set the IDAU (SPU) configuration to allow
Non-Secure access to all CPU resources utilized by the Non-Secure application
firmware. SPU configuration shall take place before jumping to the Non-Secure
application.

Building a Secure only application
==================================

Build the Zephyr app in the usual way (see :ref:`build_an_application`
and :ref:`application_run`), using ``-DBOARD=nrf7002dk/nrf5340/cpuapp`` for
the firmware running on the nRF5340 application core, and using
``-DBOARD=nrf7002dk/nrf5340/cpunet`` for the firmware running
on the nRF5340 network core.

Flashing
========

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`. Then you can build and flash
applications as usual (:ref:`build_an_application` and
:ref:`application_run` for more details).

.. warning::

   The nRF5340 has a flash read-back protection feature. When flash read-back
   protection is active, you will need to recover the chip before reflashing.
   If you are flashing with :ref:`west <west-build-flash-debug>`, run
   this command for more details on the related ``--recover`` option:

   .. code-block:: console

      west flash -H -r nrfjprog --skip-rebuild

.. note::

   Flashing and debugging applications on the nRF5340 DK requires
   upgrading the nRF Command Line Tools to version 10.12.0. Further
   information on how to install the nRF Command Line Tools can be
   found in :ref:`nordic_segger_flashing`.

Here is an example for the :zephyr:code-sample:`hello_world` application running on the
nRF5340 application core.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the board nRF7002 DK
can be found. For example, under Linux, :code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nrf7002dk/nrf5340/cpuapp
   :goals: build flash

Debugging
=========

Refer to the :ref:`nordic_segger` page to learn about debugging Nordic
boards with a Segger IC.

Next steps
**********

You have now completed getting started with the nRF7002 DK.
See the following links for where to go next:

* `Installation`_ and `Configuring and Building`_ documentation to install the
  nRF Connect SDK and learn more about its development environment.
* `Developing with nRF70`_ documentation for more advanced topics related to the nRF70 Series.
* `Wi-Fi`_ documentation for information related to Wi-Fi protocol and Wi-Fi modes of operation.

References
**********

.. target-notes::

.. _Wi-Fi Certification program:
   https://www.wi-fi.org/certification
.. _UG Wi-Fi certification:
   https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/protocols/wifi/wifi_certification.html#ug-wifi-certification
.. _IDAU:
   https://developer.arm.com/docs/100690/latest/attribution-units-sau-and-idau
.. _nRF7002 DK website:
   https://www.nordicsemi.com/Products/Development-hardware/nrf7002-dk
.. _nRF7002 DK Product Specification:
   https://docs.nordicsemi.com/bundle/ps_nrf5340/page/keyfeatures_html5.html
.. _Trusted Firmware M:
   https://www.trustedfirmware.org/projects/tf-m/
.. _Installation:
   https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/installation.html#installation
.. _Configuring and Building:
   https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/app_dev/config_and_build/index.html#configuration-and-build
.. _Developing with nRF70:
   https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/app_dev/device_guides/nrf70/index.html#ug-nrf70-developing
.. _Wi-Fi:
   https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/protocols/wifi/index.html#ug-wifi
