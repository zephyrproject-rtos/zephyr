.. zephyr:code-sample:: console_echo
   :name: Console echo
   :relevant-api: console_api

   Echo an input character back to the output using the Console API.

Overview
********

This example shows how the :c:func:`console_getchar` and
:c:func:`console_putchar` functions can be used to echo an input character
back to the console.

Requirements
************

UART console is required to run this sample.

Building and Running
********************

Build and flash the sample as follows, changing ``nucleo_f401re`` for your
board. Alternatively you can run this using QEMU, as described in
:zephyr:code-sample:`console_getchar`.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/console/echo
   :host-os: unix
   :board: nucleo_f401re
   :goals: build flash
   :compact:

Following the initial prompt given by the firmware, start pressing keys on a
keyboard, and they will be echoed back to the terminal program you are using.

Running on QEMU
===============

On QEMU you can use the VIRTIO console device. It is already configured in
Zephyr. To use it, run:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/console/echo
   :board: qemu_x86_64
   :gen-args: -DDTC_OVERLAY_FILE=virtconsole.overlay -DEXTRA_CONF_FILE=overlay-virtconsole.conf
   :goals: run
   :compact:

It is possible to use multiple inputs/outputs at once by configuring them
in the ``overlay-virtconsole.conf`` file.

* Add ``CONFIG_UART_VIRTIO_CONSOLE_F_MULTIPORT=y`` to enable the
  multiport feature.
* Configure more ports using the ``CONFIG_QEMU_EXTRA_FLAGS`` option.
  Refer to the QEMU manual for details.

Sample Output
=============

.. code-block:: console

    You should see another line with instructions below. If not,
    the (interrupt-driven) console device doesn't work as expected:
    Start typing characters to see them echoed back
