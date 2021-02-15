.. _nrf-ieee802154-rpmsg-sample:

nRF IEEE 802.15.4: Serialization RPMsg
######################################

Overview
********

This sample exposes IEEE 802.15.4 radio driver support
to another device or CPU using the RPMsg transport which is
a part of `OpenAMP <https://github.com/OpenAMP/open-amp/>`__.

Requirements
************

* A board with nRF53 SoC

Building and Running
********************

This sample can be found under :zephyr_file:`samples/boards/nrf/ieee802154/802154_rpmsg`
in the Zephyr tree.

To use this application, you need a board with nRF53 SoC.
You can then build this application and flash it onto your board in
the usual way. See :ref:`boards` for board-specific building and
programming information.

To test this sample, you need a separate device/CPU that acts as 802.15.4
serialization RPMsg peer.
This sample is compatible with the Nordic 802.15.4 serialization. See the
:option:`CONFIG_NRF_802154_SER_HOST` configuration option for more information.

The sample is configured to be energy efficient by disabling:
 * Serial Console (in :file:`prj.conf`)
 * Logger (in :file:`prj.conf`)
