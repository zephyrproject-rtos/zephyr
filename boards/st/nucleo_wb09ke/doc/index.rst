.. zephyr:board:: nucleo_wb09ke

Overview
********

The Nucleo WB09KE board is a Bluetooth |reg| Low Energy wireless and ultra-low-power
board featuring an ARM Cortex |reg|-M0+ based STM32WB09KEV MCU, embedding a
powerful and ultra-low-power radio compliant with the Bluetooth® Low Energy
SIG specification v5.4.

More information about the board can be found on the `Nucleo WB09KE webpage`_.

Hardware
********

Nucleo WB09KE provides the following hardware components:

- STM32WB09KEV in VFQFPN32 package
- ARM |reg| 32-bit Cortex |reg|-M0+ CPU
- 64 MHz maximal CPU frequebct
- 512 KB Flash
- 64 KB SRAM

More information about STM32WB09KEV can be found here:

- `WB09KE on www.st.com`_
- `STM32WB09 reference manual`_


Supported Features
==================

.. zephyr:board-supported-hw::

Bluetooh support
----------------

BLE support is enabled; however, to build a Zephyr sample using this board,
you first need to fetch the Bluetooth controller library into Zephyr as a binary BLOB.

To fetch binary BLOBs:

.. code-block:: console

   west blobs fetch hal_stm32

Connections and IOs
===================

Default Zephyr Peripheral Mapping:
----------------------------------

- USART1 TX/RX       : PA1/PB0 (ST-Link Virtual COM Port)
- BUTTON (B1)        : PA0
- BUTTON (B2)        : PB5
- BUTTON (B3)        : PB14
- LED (LD1/BLUE)     : PB1
- LED (LD2/GREEN)    : PB4
- LED (LD3/RED)      : PB2

For more details, please refer to the `Nucleo WB09KE board User Manual`_.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Nucleo WB09KE board includes an ST-LINK-V3EC embedded debug tool interface.

Applications for the ``nucleo_w09ke`` board target can be built and flashed
in the usual way (see :ref:`build_an_application` and :ref:`application_run`
for more details).

Flashing
========

The board is configured to be flashed using the west `STM32CubeProgrammer`_ runner,
so :ref:`it must be installed <stm32cubeprog-flash-host-tools>` beforehand.

Alternatively, OpenOCD can also be used to flash the board using the
``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd

Flashing an application to Nucleo WB09KE
----------------------------------------

Connect the Nucleo WB09KE to your host computer using the USB port,
then run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Now build and flash an application. Here is an example for
:zephyr:code-sample:`hello_world`.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_wb09ke
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! nucleo_wb09ke/stm32wb09


Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_wb09ke
   :maybe-skip-config:
   :goals: debug

.. warning::
   Debugging applications on this board may require usage of `STMicroelectronics's OpenOCD fork`_.
   `STM32CubeIDE`_ version 1.16.0 and higher provide the OpenOCD binary and scripts allowing debugging on STM32WB09.

   Assuming CubeIDE is installed at ``${STM32CubeIDE}``, the following command can be used to debug an application on this board:

   .. code-block:: console

      $ west debug --openocd="${STM32CubeIDE}/plugins/com.st.stm32cube.ide.mcu.externaltools.openocd.PLAT_X.X.XXX.XXXXXXXXXXXX/tools/bin/openocd"
                   --openocd-search="${STM32CubeIDE}/plugins/com.st.stm32cube.ide.mcu.debug.openocd_X.X.XXX.XXXXXXXXXXXX/resources/openocd/st_scripts"

   Note that the ``PLAT`` and ``X.X.XXX.XXXXXXXXXXXX`` parts of the command change with the STM32CubeIDE version. You can find the correct
   values by searching in ``${STM32CubeIDE}/plugins``, knowing that the leading ``com.st[...]`` part of directories' names never changes.

.. _`Nucleo WB09KE webpage`:
   https://www.st.com/en/evaluation-tools/nucleo-wb09ke.html

.. _`WB09KE on www.st.com`:
   https://www.st.com/en/microcontrollers-microprocessors/stm32wb09ke.html

.. _`STM32WB09 reference manual`:
   https://www.st.com/resource/en/reference_manual/rm0505-stm32wb09xe-ultralow-power-wireless-32bit-mcu-armbased-cortexm0-with-bluetooth-low-energy-and-24-ghz-radio-solution-stmicroelectronics.pdf

.. _`Nucleo WB09KE board User Manual`:
   https://www.st.com/resource/en/user_manual/um3345-stm32wb09-nucleo64-board-mb1801-and-mb2032-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html

.. _`STMicroelectronics's OpenOCD fork`:
   https://github.com/STMicroelectronics/OpenOCD

.. _`STM32CubeIDE`:
   https://www.st.com/en/development-tools/stm32cubeide.html
