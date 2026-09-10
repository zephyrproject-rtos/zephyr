.. zephyr:board:: nucleo_f439zi

Overview
********

The Nucleo F439ZI board features an ARM Cortex-M4 based STM32F439ZI MCU
with a wide range of connectivity support and configurations. This SoC
is basically a clone of the STM32F429ZI with a supplementary hardware
cryptographic accelerator.

More information about STM32F439ZI can be found here:

- `STM32F439ZI on www.st.com`_
- `STM32F439 reference manual`_
- `STM32F439 datasheet`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Nucleo F439ZI board includes an ST-LINK/V2-1 embedded debug tool interface.

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD or JLink can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink

Flashing an application to the Nucleo F439ZI
--------------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

Run a serial host program to connect with your board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_f439zi
   :goals: build flash

You should see user led "LD1" blinking.

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_f439zi
   :maybe-skip-config:
   :goals: debug

.. target-notes::
.. _STM32F439ZI on www.st.com:
   https://www.st.com/en/microcontrollers/stm32f439zi.html

.. _STM32F439 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00031020.pdf

.. _STM32F439 datasheet:
   https://www.st.com/resource/en/datasheet/stm32f439zi.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
