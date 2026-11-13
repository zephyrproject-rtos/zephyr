.. zephyr:code-sample:: rc-receive
   :name: Remote control receive
   :relevant-api: rc_interface

   Print the events decoded from an infrared remote control.

Overview
********

The sample takes the rc device behind the ``rc0`` devicetree alias, enables
the NEC protocol, starts reception and prints every decoded event with its
protocol, scancode and key state (press, repeat or release).

With :kconfig:option:`CONFIG_SAMPLE_RC_RAW` enabled, it also prints the raw
pulse and space words received by the device.

On :zephyr:board:`native_sim`, the ``rc0`` alias points to a loopback device
and :kconfig:option:`CONFIG_SAMPLE_RC_SELFTEST` sends one NEC frame through it
at start, so the sample prints an event without hardware.

Requirements
************

A board with an ``rc0`` alias to an rc device, for example a demodulating IR
receiver module such as the VS1838B on a GPIO:

* VCC to the board supply that matches the GPIO level, within the module
  supply range.
* GND to ground.
* OUT to a GPIO input.

Use the pin order of the module or part datasheet. The receiver output is
low while the carrier is present, so the GPIO is ``GPIO_ACTIVE_LOW``:

.. code-block:: devicetree

   / {
           aliases {
                   rc0 = &ir_rx;
           };

           ir_rx: ir-receiver {
                   compatible = "gpio-ir-receiver";
                   gpios = <&gpio0 5 GPIO_ACTIVE_LOW>;
           };
   };

For m1s_dock, see the overlay in ``boards/``.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/rc/receive
   :board: native_sim
   :goals: build run
   :compact:

Sample Output
*************

.. code-block:: console

   rc receive sample on rc-loopback
   rc-loopback: nec scancode 0x0102 press
   rc-loopback: nec scancode 0x0102 release
