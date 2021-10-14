.. _usb_hid-cdc:

USB HID CDC ACM Application
###########################

Overview
********

This sample app demonstrates use of multiple USB classes with multiple
instances. It combines two HID instances and two CDC ACM instances.
This sample can be found under :zephyr_file:`samples/subsys/usb/hid-cdc` in the
Zephyr project tree.

Requirements
************

This project requires an USB device driver and multiple endpoints.

The board hardware must have a push button connected via a GPIO pin. These are
called "User buttons" on many of Zephyr's :ref:`boards`.

The button must be configured using the ``sw0`` :ref:`devicetree <dt-guide>`
alias, usually in the :ref:`BOARD.dts file <devicetree-in-out-files>`. You will
see this error if you try to build this sample for an unsupported board:

.. code-block:: none

   Unsupported board: sw0 devicetree alias is not defined

You may see additional build errors if the ``sw0`` alias exists, but is not
properly defined.

If the devicetree aliases ``sw1``, ``sw2``, and ``sw3`` are defined, they will
also be used as additional buttons as described below.

Building and Running
********************

This sample can be built for multiple boards. To build and flash it
for the :ref:`nrf52840dk_nrf52840` board:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/usb/hid-cdc
	:board: nrf52840dk_nrf52840
	:goals: build flash
	:compact:

After you have built and flashed the sample app image to your board, plug the
board into a host device, for example, a PC running Linux.
Two CDC ACM interfaces (for example /dev/ttyACM1 and /dev/ttyACM2)
and two HID devices will be detected:

.. code-block:: console

	usb 2-2: new full-speed USB device number 3 using ohci-pci
	usb 2-2: New USB device found, idVendor=2fe3, idProduct=0003, bcdDevice= 2.03
	usb 2-2: New USB device strings: Mfr=1, Product=2, SerialNumber=3
	usb 2-2: Product: Zephyr HID and CDC ACM sample
	usb 2-2: Manufacturer: ZEPHYR
	usb 2-2: SerialNumber: 86FE679A598AC47A
	cdc_acm 2-2:1.0: ttyACM1: USB ACM device
	input: ZEPHYR Zephyr HID and CDC ACM sample as /devices/pci0000:00/0000:00:06.0/usb2/2-2/2-2:1.2/0003:2FE3:0100.0002/input/input8
	hid-generic 0003:2FE3:0100.0002: input,hidraw1: USB HID v1.10 Mouse [ZEPHYR Zephyr HID and CDC ACM sample] on usb-0000:00:06.0-2/input2
	cdc_acm 2-2:1.3: ttyACM2: USB ACM device
	input: ZEPHYR Zephyr HID and CDC ACM sample as /devices/pci0000:00/0000:00:06.0/usb2/2-2/2-2:1.5/0003:2FE3:0100.0003/input/input9
	hid-generic 0003:2FE3:0100.0003: input,hidraw2: USB HID v1.10 Keyboard [ZEPHYR Zephyr HID and CDC ACM sample] on usb-0000:00:06.0-2/input5

You can now connect to both CDC ACM ports:

.. code-block:: console

	minicom -D /dev/ttyACM1 -b 115200

.. code-block:: console

	minicom -D /dev/ttyACM2 -b 115200

After both ports have been connected to, messages explaining usage of each port will be displayed:

.. code-block:: console

	Welcome to CDC_ACM_0
	Supported commands:
	up    - moves the mouse up
	down  - moves the mouse down
	right - moves the mouse to right
	left  - moves the mouse to left

.. code-block:: console

	Welcome to CDC_ACM_1
	Enter a string and terminate it with ENTER.
	It will be sent via HID when BUTTON 2 is pressed.
	You can modify it by sending a new one here.

CDC ACM 0 may be used to control the mouse by typing a command and pressing :kbd:`ENTER`.

CDC ACM 1 is used to control the keyboard - any string typed into it and finished with :kbd:`ENTER` will be saved
on the device and typed back to the host when BUTTON 2 is pressed.

Buttons have following functions:

- Button 0 moves HID mouse in random direction
- Button 1 is a left HID mouse button
- Button 2 types the string sent with CDC ACM 1 using HID keyboard
- Button 3 is a CAPS LOCK on HID keyboard
