.. zephyr:code-sample:: tracing
   :name: Tracing

   Send tracing formatted packet to the host with supported backends.

This application can be used to demonstrate the tracing feature. The tracing
formatted packet will be sent to the host with the currently supported tracing
backend under tracing generic infrastructure.

Requirements
************

Depends of the boards which you are using, choose one of .conf files for use tracing subsys.

Usage for UART Tracing Backend
******************************

Build a UART-tracing image with:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: mps2_an521
	:conf: "prj_uart.conf"
	:goals: build
	:compact:

or:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: mps2_an521
	:conf: "prj_uart_ctf.conf"
	:goals: build
	:compact:

.. note:: You may need to set "zephyr,tracing-uart" property under the chosen node in your devicetree. See :zephyr_file:`boards/mps2_an521.overlay` for an example.

After the application has run for a while, check the trace output file.

Usage for USB Tracing Backend
*****************************

Build a USB-tracing image with:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: sam_e70_xplained
	:conf: "prj_usb.conf"
	:goals: build
	:compact:

or:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: sam_e70_xplained
	:conf: "prj_usb_ctf.conf"
	:goals: build
	:compact:

After the serial console has stable output like this:

.. code-block:: console

	threadA: Hello World!
	threadB: Hello World!
	threadA: Hello World!
	threadB: Hello World!

Connect the board's USB port to the host device and
run the :zephyr_file:`scripts/tracing/trace_capture_usb.py` script on the host:

.. code-block:: console

	sudo python3 trace_capture_usb.py -v 0x2FE9 -p 0x100 -o channel0_0

The VID and PID of USB device can be configured, just adjusting it accordingly.

Usage for POSIX Tracing Backend
*******************************

Build a POSIX-tracing image with:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: native_posix
	:conf: "prj_native_posix.conf"
	:goals: build
	:compact:

or:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: native_posix
	:conf: "prj_native_posix_ctf.conf"
	:goals: build
	:compact:

After the application has run for a while, check the trace output file.

Usage for USER Tracing Backend
*******************************

Build a USER-tracing image with:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: qemu_x86
	:conf: "prj_user.conf"
	:goals: build
	:compact:

After the application has run for a while, check the trace output file.
