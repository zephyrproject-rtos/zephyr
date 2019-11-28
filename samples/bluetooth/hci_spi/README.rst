.. _bluetooth-hci-spi-sample:

Bluetooth: HCI SPI
##################

Overview
********

Expose Zephyr Bluetooth Controller support over SPI to another device/CPU using
the Zephyr SPI HCI transport protocol (similar to BlueNRG).

Requirements
************

* A board with SPI slave, GPIO and BLE support.

Building and Running
********************

In order to use this application, you need a board with a Bluetooth
controller and SPI slave drivers, and a spare GPIO to use as an
interrupt line to the SPI master.

You then need to ensure that your :ref:`device-tree` settings provide a
definition for the slave HCI SPI device::

	bt-hci@0 {
		compatible = "zephyr,bt-hci-spi-slave";
		...
	};

You can then build this application and flash it onto your board in
the usual way; see :ref:`boards` for board-specific building and
flashing information.

You will also need a separate chip acting as BT HCI SPI master. This
application is compatible with the HCI SPI master driver provided by
Zephyr's Bluetooth HCI driver core; see the help associated with the
BT_SPI configuration option for more information.

Refer to :ref:`bluetooth-samples` for general Bluetooth information, and
to :ref:`96b_carbon_nrf51_bluetooth` for instructions specific to the
96Boards Carbon board.
