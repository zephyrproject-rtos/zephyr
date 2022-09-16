.. _blink-led-sample:
.. _pwm-blinky-sample:

PWM Blinky
##########

Overview
********

This application blinks a LED using the :ref:`PWM API <pwm_api>`. See
:ref:`blinky-sample` for a GPIO-based sample.

The LED starts blinking at a 1 Hz frequency. The frequency doubles every 4
seconds until it reaches 128 Hz. The frequency will then be halved every 4
seconds until it returns to 1 Hz, completing a single blinking cycle. This
faster-then-slower blinking cycle then repeats forever.

Some PWM hardware cannot set the PWM period to 1 second to achieve the blinking
frequency of 1 Hz. This sample calibrates itself to what the hardware supports
at startup. The maximum PWM period is decreased appropriately until a value
supported by the hardware is found.

Requirements
************

The board must have an LED connected to a PWM output channel. The PWM
controlling this LED must be configured using the ``pwm_led0`` :ref:`devicetree
<dt-guide>` alias, usually in the :ref:`BOARD.dts file
<devicetree-in-out-files>`.

Wiring
******

No additional wiring is necessary if ``pwm_led0`` refers to hardware that is
already connected to an LED on the board.

In these other cases, however, manual wiring is necessary:

.. list-table::
   :header-rows: 1

   * - Board
     - Wiring
   * - :ref:`nucleo_f401re_board`
     - connect PWM2 (PA0) to an LED
   * - :ref:`nucleo_l476rg_board`
     - connect PWM2 (PA0) to an LED
   * - :ref:`stm32f4_disco_board`
     - connect PWM2 (PA0) to an LED
   * - :ref:`nucleo_f302r8_board`
     - connect PWM2 (PA0) to an LED
   * - :ref:`nucleo_f103rb_board`
     - connect PWM1 (PA8) to an LED
   * - :ref:`nucleo_wb55rg_board`
     - connect PWM1 (PA8) to an LED

Building and Running
********************

To build and flash this sample for the :ref:`nrf52840dk_nrf52840`:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky_pwm
   :board: nrf52840dk_nrf52840
   :goals: build flash
   :compact:

Change ``nrf52840dk_nrf52840`` appropriately for other supported boards.

After flashing, the sample starts blinking the LED as described above. It also
prints information to the board's console.
