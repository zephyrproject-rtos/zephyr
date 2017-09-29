.. _bluetooth_hci_spi:

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
interrupt line to the SPI master. Currently, only the legacy SPI API
is supported by this sample application.

You then need to ensure that your :ref:`application_configuration`
provides the Kconfig values defining these peripherals:

- BT_CTLR_TO_HOST_SPI_DEV_NAME: name of the SPI device on your
  board, which interfaces in slave mode with the BT HCI SPI driver.

- BT_CTLR_TO_HOST_SPI_IRQ_DEV_NAME: name of the GPIO device
  which contains the interrupt pin to the SPI master.

- BT_CTLR_TO_HOST_SPI_IRQ_PIN: pin number on the GPIO device to
  use as an interrupt line to the SPI master.

You can then build this application and flash it onto your board in
the usual way; see :ref:`boards` for board-specific building and
flashing information.

You will also need a separate chip acting as BT HCI SPI master. This
application is compatible with the HCI SPI master driver provided by
Zephyr's Bluetooth HCI driver core; see the help associated with the
BT_SPI configuration option for more information.

Refer to :ref:`bluetooth_setup` for general Bluetooth information, and
to :ref:`96b_carbon_nrf51_bluetooth` for instructions specific to the
96Boards Carbon board.
