.. _96b_carbon_multi_thread_blinky:

96Boards Carbon Basic Multi Thread Example
##########################################

Overview
********

This example demonstrates spawning of multiple threads using K_THREAD_DEFINE.

The example works by spawing  three threads, each controlling a separate LED
on the 96Boards Carbon board. Each of these LEDs (USR1, USR2 and BT) have
their individual loop control and timing logic defined by separate functions.

- blink1() controls the USR1 LED that has a 100ms sleep cycle
- blink2() controls the USR2 LED that has a 1000ms sleep cycle
- blink3() controls the BT LED that increases sleep by 100ms every cycle
	 upto 600ms and then resets to 100ms


Each thread is then defined at compile time using K_THREAD_DEFINE.

Apart from that, a main() thread is also present that provides a basic shell
based on the shell sample at <zephyr_base>/samples/subsys/shell/shell/

Building
********

.. code-block:: console

    $ cd samples/boards/96b_carbon/multi_thread_blinky
    $ make

Flashing via OTG

.. code-block:: console

    $ sudo dfu-util -d [0483:df11] -a 0 -D outdir/96b_carbon/zephyr.bin -s 0x08000000

Flash via UART

.. code-block:: console

    $ sudo stm32flash -b 115200 -g 0x0 -w outdir/96b_carbon/zephyr.bin /dev/ttyUSB0
