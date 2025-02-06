.. zephyr:board:: xg29_rb4412a

Overview
********

The xG24-RB4412A radio board provides support for the Silicon Labs EFR32MG29 SoC.

Hardware
********

- EFR32MG29B140F1024IM40 SoC
- CPU core: ARM Cortex®-M33 with FPU
- Flash memory: 1024 kB
- RAM: 256 kB
- Transmit power: up to +8 dBm
- Operation frequency: 2.4 GHz
- Crystal oscillators for LFXO (32.768 kHz) and HFXO (38.4 MHz)

Supported Features
==================

The ``xg29_rb4412a`` board target supports the following hardware features:

+-----------+------------+------------------------+
| Interface | Controller | Driver/Component       |
+===========+============+========================+
| CMU       | on-chip    | clock control          |
+-----------+------------+------------------------+
| MSC       | on-chip    | flash                  |
+-----------+------------+------------------------+
| GPIO      | on-chip    | gpio, pin control      |
+-----------+------------+------------------------+
| RTCC      | on-chip    | system clock, counter  |
+-----------+------------+------------------------+
| MPU       | on-chip    | memory protection unit |
+-----------+------------+------------------------+
| NVIC      | on-chip    | interrupt controller   |
+-----------+------------+------------------------+
| USART     | on-chip    | serial, spi            |
+-----------+------------+------------------------+
| EUSART    | on-chip    | serial, spi            |
+-----------+------------+------------------------+
| I2C       | on-chip    | i2c                    |
+-----------+------------+------------------------+
| LDMA      | on-chip    | dma                    |
+-----------+------------+------------------------+
| WDOG      | on-chip    | watchdog               |
+-----------+------------+------------------------+
| SE        | on-chip    | entropy                |
+-----------+------------+------------------------+
| RADIO     | on-chip    | bluetooth              |
+-----------+------------+------------------------+
| ACMP      | on-chip    | comparator             |
+-----------+------------+------------------------+

Programming and Debugging
*************************

Applications for the ``xg29_rb4412a`` board target can be built, flashed, and debugged in the
usual way. See :ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Flashing
========

As an example, this section shows how to build and flash the :zephyr:code-sample:`hello_world`
application.

To build and program the sample to the xG24-RB4412A, complete the following steps:

First, plug the xG24-RB4412A to a compatible mainboard and connect the mainboard to your computer
using the USB port on the left side.
Next, build and flash the sample by running the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xg29_rb4412a
   :goals: build flash

``west flash`` will by default use SEGGER JLink. Make sure that the JLinkExe binary is available on
the PATH. Alternatively, use ``west flash -r silabs_commander`` to use Simplicity Commander to flash.
In this case, make sure that the commander binary is available on PATH.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! xg29_rb4412a

Bluetooth
=========

To use the BLE function, run the command below to retrieve necessary binary
blobs from the SiLabs HAL repository.

.. code-block:: console

   west blobs fetch hal_silabs

Then build the Zephyr kernel and a Bluetooth sample with the following
command. The :zephyr:code-sample:`bluetooth_observer` sample application is used in
this example.

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/observer
   :board: xg29_rb4412a
   :goals: build
