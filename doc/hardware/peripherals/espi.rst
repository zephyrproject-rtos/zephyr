.. _espi_api:

eSPI
####

Overview
********

The eSPI (enhanced serial peripheral interface) is a serial bus that is
based on SPI. It also features a four-wire interface (receive, transmit, clock
and slave select) and three configurations: single IO, dual IO and quad IO.

The technical advancements include lower voltage signal levels (1.8V vs. 3.3V),
lower pin count, and the frequency is twice as fast (66MHz vs. 33MHz)
Because of its enhancements, the eSPI is used to replace the LPC
(lower pin count) interface, SPI, SMBus and sideband signals.

See `eSPI interface specification`_ for additional details.

API Reference
*************

.. doxygengroup:: espi_interface

.. _eSPI interface specification:
    https://www.intel.com/content/dam/support/us/en/documents/software/chipset-software/327432-004_espi_base_specification_rev1.0_cb.pdf

The API for the eSPI bus is declared in the file: `include/zephyr/drivers/espi/espi.h`

Testing
*******

To test the eSPI bus there's need to allow communication of the microcontroller, being the
peripheral on the bus, to the host of the bus.
It's possible thanks to the emulators that can be used instead of real peripherals or controllers
connected to the bus.

There are two emulators that can be used together or one of them can be used independently.
Controller emulator is counterpart of the controller in the microcontroller and can be used
independently because it can exist without anything connected to it. Second one, host emulator,
represents the host of the eSPI bus that is connected to the microcontroller and transfers data
between themselves. It can't exist without controller emulator because it can't communicate without
bus existing in the microcontroller.

The eSPI API was extended to support calls and callbacks to/from the host emulator.
This API is declared in the header file: `include/zephyr/drivers/espi/espi_emul.h`
The first parameter, being a pointer to the device should always be a device of controller emulator,
not the host emulator. The handlers know which functions should be handled by controller emulator
and which by host emulator if one exists.

eSPI controller emulator
------------------------

Compatible string for the device tree node is `zephyr,espi-emul-controller`

The devicetree node that enables this emulator for tests:

.. literalinclude:: ../../../boards/posix/native_posix/native_posix.dts
   :language: dts
   :start-at: espi0: espi@300
   :end-at: };
   :linenos:

It's enabled using the Kconfig :kconfig:option:`CONFIG_ESPI_EMUL`
This emulator has support for the Intel 8042 keyboard controller. It can be enabled by Kconfig
:kconfig:option:`CONFIG_ESPI_EMUL_KBC8042`
The calls to the Read/Write LPC requests that are targetted to the keyboard controller are
handled by the emulator, as if the keyboard controller was a port of the microcontroller running
the zephyr application. If host emulator is connected to the controller emulator, these calls will
inform the host emulator about data available to read.
It has also support for the shared memory enabled by Kconfig
:kconfig:option:`CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION`
It allows to communicate by eSPI peripheral API from the microcontroller side and by the I/O
port operations from the host emulator.

eSPI host emulator
------------------

Compatible string for the device tree node is `zephyr,espi-emul-host``

The devicetree node that enables this emulator for tests:

.. literalinclude:: ../../../tests/drivers/espi/boards/native_posix.overlay
   :language: dts
   :lines: 7-
   :linenos:

.. note::
   If you are adding a callback using the object `struct espi_callback` created on the stack,
   remember to always remove the callback using API function regardless of the result of tests.
   When object is destroyed without being removed from the callbacks list, it may corrupt the
   stack and dereference invalid memory. It will result in crash of application.

This emulator can be used to verify that application handles properly events from the eSPI bus.
It includes changes to the virtual-wire signals, both sent from the MCU and from the host, and also
the communication to and from the keyboard controller, if it is enabled by Kconfig.

The I/O operations can be sent from the host through the emulated eSPI bus using the functions
`espi_emul_host_io_read` and `espi_emul_host_io_write`.

The changes to the virtual-wire signals can be done using the function `espi_emul_host_set_vwire`.
The emulators know which signal is designed to go from the host to the controller and which other
way, so it automatically send event to the apropriate receiver.

The eSPI API supports callbacks for the microcontroller, which should be fired after receiving the
data from the host. However, in the emulated environment, there is also a need to verify if data
from the controller is sent properly to the host. To allow this operation, the additional function
is added that manages the callbacks on the emulated host side: `espi_emul_host_manage_callback`.

The callbacks registered to the controller emulator can be triggered using the function
`espi_emul_raise_event`. It is used internally to trigger callbacks from the host side, as a
response to the data that normally should be received by the microcontroller from the eSPI bus.
