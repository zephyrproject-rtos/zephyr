.. _bluetooth-hci-spi-sample:

Bluetooth: HCI SPI
##################

Overview
********

Expose Zephyr Bluetooth Controller support over SPI to another device/CPU using
the Zephyr SPI HCI transport protocol (similar to BlueNRG).

Requirements
************

A board with SPI slave, GPIO and Bluetooth Low Energy support.

Building and Running
********************

You then need to ensure that your :ref:`devicetree <dt-guide>` defines a node
for the HCI SPI slave device with compatible
:dtcompatible:`zephyr,bt-hci-spi-slave`. This node sets an interrupt line to
the host and associates the application with a SPI bus to use.

See :zephyr_file:`boards/nrf51dk_nrf51422.overlay
<samples/bluetooth/hci_spi/boards/nrf51dk_nrf51422.overlay>` in this sample
directory for an example overlay for the :ref:`nrf51dk_nrf51422` board.

You can then build this application and flash it onto your board in
the usual way; see :ref:`boards` for board-specific building and
flashing information.

You will also need a separate chip acting as BT HCI SPI master. This
application is compatible with the HCI SPI master driver provided by
Zephyr's Bluetooth HCI driver core; see the help associated with the
:kconfig:option:`CONFIG_BT_SPI` configuration option for more information.

Refer to :ref:`bluetooth-samples` for general Bluetooth information, and
to :ref:`96b_carbon_nrf51_bluetooth` for instructions specific to the
96Boards Carbon board.
