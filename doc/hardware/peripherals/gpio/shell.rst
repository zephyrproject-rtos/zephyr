.. _gpio_shell:

GPIO Shell
##########

.. contents::
    :local:
    :depth: 1

Overview
********

.. zephyr:shell-module:: gpio
   :kconfig: CONFIG_GPIO_SHELL
   :depends: CONFIG_GPIO

   The GPIO shell provides a ``gpio`` command with a set of subcommands for the :ref:`shell
   <shell_api>` module, which allow configuring, reading, writing and toggling the pins of any GPIO
   controller through an interactive interface, without having to write a dedicated application.

   This makes it a convenient way to check the wiring of a board, or to experiment with the
   configuration of a pin, before (or while) writing application code using the :ref:`gpio_api`.

The following :ref:`Kconfig <kconfig>` options, all enabled by default, control which subcommands
are available:

* :kconfig:option:`CONFIG_GPIO_SHELL_INFO_CMD` enables the ``gpio info`` subcommand.
* :kconfig:option:`CONFIG_GPIO_SHELL_TOGGLE_CMD` enables the ``gpio toggle`` subcommand.
* :kconfig:option:`CONFIG_GPIO_SHELL_BLINK_CMD` enables the ``gpio blink`` subcommand.

.. note::

   The examples below were captured on :zephyr:board:`native_sim`, using a devicetree overlay
   giving names to some of the pins of its emulated GPIO controller and reserving others. On a
   real board, expect device names such as ``gpio@50000000`` and pin names taken from the board's
   devicetree.

Listing controllers and pins
****************************

The :zephyr:shell-command:`gpio devices` subcommand lists the GPIO controllers available on the
board. A controller can be referred to either by its device name or by any of its devicetree node
labels, both of which support tab completion:

.. code-block:: console

   uart:~$ gpio devices
   Device           Other names
   gpio_emul        gpio0

The :zephyr:shell-command:`gpio info` subcommand prints the pins of a controller, along with their
line name (taken from the ``gpio-line-names`` devicetree property of the controller) and whether
they are reserved (``gpio-reserved-ranges`` property):

.. code-block:: console

   uart:~$ gpio info gpio0
    ngpios: 8
    Reserved pin mask: 0xFFFFFF60

    Reserved  Pin  Line Name
               0    LED0
               1    LED1
               2
               3    BUTTON0
               4
        *      5
        *      6
               7

Without a controller argument, ``gpio info`` lists the pins of all the GPIO controllers instead,
sorted by line name:

.. code-block:: console

   uart:~$ gpio info
     Line         Reserved Device           Pin
                            gpio0             2
                            gpio0             4
                   *        gpio0             5
                   *        gpio0             6
                            gpio0             7
      BUTTON0               gpio0             3
      LED0                  gpio0             0
      LED1                  gpio0             1

.. tip::

   Wherever a pin number is expected, a pin can also be referred to by its line name, with any
   space replaced by an underscore. Both pin numbers and line names support tab completion.

Configuring pins
****************

A pin must be configured as an input or an output with the :zephyr:shell-command:`gpio conf`
subcommand before it can be used. The configuration is given as a compact string of flags:

* ``i`` or ``o``: input or output (mandatory).
* ``u`` or ``d``: enable the internal pull-up or pull-down resistor.
* ``h`` or ``l``: active high (the default) or active low logic.
* ``0`` or ``1``: initial logic level of an output (defaults to ``0``).

For example, to configure pin 0 as an active-high output, pin 3 as an input with a pull-up, and
the pin named ``LED1`` as an active-low output initially set to logic ``1`` (i.e. physically low):

.. code-block:: console

   uart:~$ gpio conf gpio0 0 oh
   uart:~$ gpio conf gpio0 3 iu
   uart:~$ gpio conf gpio0 LED1 ol1

Vendor-specific configuration flags (those within the ``0xFF00`` mask, see the headers in
:zephyr_file:`include/zephyr/dt-bindings/gpio`) can be passed as an additional numeric argument.

Reserved pins cannot be configured, nor driven:

.. code-block:: console

   uart:~$ gpio conf gpio0 5 o
   Reserved pin
   conf - Configure GPIO pin
   Usage: conf <device> <pin> <configuration <i|o>[u|d][h|l][0|1]> [vendor
   specific]
   ...

Reading and writing pins
************************

The :zephyr:shell-command:`gpio get` and :zephyr:shell-command:`gpio set` subcommands read and
write the *logical* level of a pin: ``1`` means active, which corresponds to a low physical level
on a pin configured as active low. :zephyr:shell-command:`gpio toggle` inverts the current level of
an output:

.. code-block:: console

   uart:~$ gpio get gpio0 3
   1
   uart:~$ gpio get gpio0 BUTTON0
   1
   uart:~$ gpio set gpio0 0 1
   uart:~$ gpio toggle gpio0 0

The :zephyr:shell-command:`gpio blink` subcommand toggles an output at 1 Hz until a key is pressed,
which is a quick way to identify a pin or an LED on a board:

.. code-block:: console

   uart:~$ gpio blink gpio0 0
   Hit any key to exit

Command reference
*****************

.. zephyr:shell-command-reference::
