.. zephyr:code-sample:: pwm-blinky
   :name: PWM Blinky
   :relevant-api: pwm_interface

   Blink an LED using the PWM API.

Overview
********

This application blinks an LED using the :ref:`PWM API <pwm_api>`. See
:zephyr:code-sample:`blinky` for a GPIO-based sample.

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
   * - :zephyr:board:`nucleo_f401re`
     - connect PWM2 (PA0) to an LED
   * - :zephyr:board:`nucleo_l476rg`
     - connect PWM2 (PA0) to an LED
   * - :zephyr:board:`stm32f4_disco`
     - connect PWM2 (PA0) to an LED
   * - :zephyr:board:`nucleo_f302r8`
     - connect PWM2 (PA0) to an LED
   * - :zephyr:board:`nucleo_f103rb`
     - connect PWM1 (PA8) to an LED
   * - :zephyr:board:`nucleo_wb55rg`
     - connect PWM1 (PA8) to an LED
   * - :zephyr:board:`esp32_devkitc_wroom`
     - connect GPIO2 to an LED
   * - :zephyr:board:`esp32s2_saola`
     - connect GPIO2 to an LED
   * - :zephyr:board:`esp32c3_devkitm`
     - connect GPIO2 to an LED

Building and Running
********************

To build and flash this sample for the :zephyr:board:`nrf52840dk`:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky_pwm
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

Change ``nrf52840dk/nrf52840`` appropriately for other supported boards.

After flashing, the sample starts blinking the LED as described above. It also
prints information to the board's console.
