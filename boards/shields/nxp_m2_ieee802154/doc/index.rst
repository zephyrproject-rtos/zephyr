.. _nxp_m2_ieee802154:

NXP M.2 IEEE 802.15.4 SPI RCP Shield
####################################

Overview
********

This Zephyr shield exposes an IEEE 802.15.4 Radio Co-Processor (RCP) over a SPI
transport, using an NXP M.2 module connected to the host platform through its M.2
interface slot. The RCP runs the 802.15.4 radio and communicates with the host
using the Spinel protocol carried over an HDLC-framed SPI link
(``spi,hdlc-rcp-if``). This makes the shield suitable for OpenThread host
applications such as the OpenThread Border Router, where the host runs the
OpenThread stack and the M.2 module provides the 802.15.4 radio.

The shield is tested with the following M.2 modules:

- Embedded Artist 2EL module - uses Murata 2EL radio module with NXP IW612 chipset
- Embedded Artist 2LL module - uses Murata 2LL radio module with NXP IW610 chipset

More information about the supported chipsets, radio modules and M.2 modules can
be found in the links below:

- `IW612 NXP Chipset <https://www.nxp.com/products/IW612>`_
- `IW610 NXP Chipset <https://www.nxp.com/products/IW610>`_
- `2EL Murata Radio Module <https://www.murata.com/en-us/products/connectivitymodule/wi-fi-bluetooth/overview/lineup/type2el>`_
- `2LL Murata Radio Module <https://www.murata.com/en-us/products/connectivitymodule/wi-fi-bluetooth/overview/lineup/type2ll>`_
- `2EL Embedded Artist Module <https://www.embeddedartists.com/products/2el-m-2-module>`_
- `2LL Embedded Artist Module <https://www.embeddedartists.com/products/2ll-m-2-module>`_

Requirements
************

To use the shield, the requirements below must be satisfied.

- An M.2 module with a SPI RCP interface and an NXP IW612 or IW610 SoC.
- A host platform with a compatible M.2 interface slot and a free SPI
  controller exposing SCK, SDI, SDO, a chip-select GPIO and an interrupt GPIO.
- The board overlay must enable the host SPI controller (for example ``&lpspi4``
  on the MIMXRT1060-EVK Rev-C) with ``cs-gpios`` and pinctrl, and instantiate the
  ``spi,hdlc-rcp-if`` device node with its ``int-gpios``.

Integration Platform
********************

This shield is validated and tested for use with the host platform listed below.
While the shield can be used with other host platforms, other combinations
are not actively tested or validated.

- :zephyr:board:`mimxrt1060_evk` Rev-C (``mimxrt1060_evk@C/mimxrt1062/qspi``).

Fetch Binary Blobs
******************

The M.2 module requires fetching binary blobs, using the following command:

.. code-block:: console

   west blobs fetch hal_nxp

Programming
***********

Use the following shield name with ``--shield <option>`` when you invoke
``west build``.

- ``nxp_m2_2el_2ll_ieee802154``: For IEEE 802.15.4 SPI RCP with NXP IW612
  (Murata 2EL) or IW610 (Murata 2LL) M.2 modules.

This single shield supports both IW612 (Murata 2EL) and IW610 (Murata 2LL) M.2
modules. The shield overlay and pin assignments are identical for both modules.

The OpenThread Border Router combines the Wi-Fi/Bluetooth M.2 shield
(``nxp_m2_2ll_wifi_bt``) with this 802.15.4 RCP shield
(``nxp_m2_2el_2ll_ieee802154``). The ``nxp_m2_2ll_wifi_bt`` shield and the NXP
Wi-Fi driver (``CONFIG_WIFI_NXP``) are required in every configuration, even when
Wi-Fi is not used as the backbone: the IW612/IW610 firmware is loaded through
the NXP Wi-Fi driver, and the M.2 module does not start without it.

Build Configurations
====================

The application is configured by three fragments passed through
``EXTRA_CONF_FILE``, all located in
``samples/net/openthread/border_router/boards/mimxrt1060_evk_mimxrt1062_qspi_C/``:

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Fragment
     - Purpose
   * - ``mimxrt1060_evk_mimxrt1062_qspi_C.conf``
     - Common RT1060 EVKC settings: NXP Wi-Fi driver, HDLC RCP host interface,
       heap and network buffer sizing, shell and logging. Always required.
   * - ``wifi.conf`` or ``eth.conf``
     - Backbone selection. ``wifi.conf`` uses the Wi-Fi STA interface as the
       infrastructure link (``CONFIG_ETH_DRIVER=n``). ``eth.conf`` uses the
       wired Ethernet port.
   * - ``iw612.conf`` or ``iw610.conf``
     - M.2 module selection: IW612 (Murata 2EL) or IW610 (Murata 2LL).

This gives four supported combinations:

.. list-table::
   :header-rows: 1
   :widths: 20 20 60

   * - Module
     - Backbone
     - Fragments
   * - IW612
     - Wi-Fi
     - ``mimxrt1060_evk_mimxrt1062_qspi_C.conf wifi.conf iw612.conf``
   * - IW612
     - Ethernet
     - ``mimxrt1060_evk_mimxrt1062_qspi_C.conf eth.conf iw612.conf``
   * - IW610
     - Wi-Fi
     - ``mimxrt1060_evk_mimxrt1062_qspi_C.conf wifi.conf iw610.conf``
   * - IW610
     - Ethernet
     - ``mimxrt1060_evk_mimxrt1062_qspi_C.conf eth.conf iw610.conf``

Wi-Fi Backbone
==============

To build for the MIMXRT1060-EVK Rev-C with an IW612 (Murata 2EL) module and a
Wi-Fi backbone:

.. code-block:: console

   west build -p -b mimxrt1060_evk@C/mimxrt1062/qspi \
       -d build_mimxrt1060_evk/border_router_iw612_wifi \
       --shield "nxp_m2_2ll_wifi_bt nxp_m2_2el_2ll_ieee802154" \
       zephyr/samples/net/openthread/border_router \
       -- -DEXTRA_CONF_FILE=" \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/mimxrt1060_evk_mimxrt1062_qspi_C.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/wifi.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/iw612.conf"

For an IW610 (Murata 2LL) module, use ``iw610.conf`` instead:

.. code-block:: console

   west build -p -b mimxrt1060_evk@C/mimxrt1062/qspi \
       -d build_mimxrt1060_evk/border_router_iw610_wifi \
       --shield "nxp_m2_2ll_wifi_bt nxp_m2_2el_2ll_ieee802154" \
       zephyr/samples/net/openthread/border_router \
       -- -DEXTRA_CONF_FILE=" \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/mimxrt1060_evk_mimxrt1062_qspi_C.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/wifi.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/iw610.conf"

Connect the Wi-Fi STA interface to the access point from the shell
(``wifi connect ...``) to bring the backbone link up.

Ethernet Backbone
=================

To build for the MIMXRT1060-EVK Rev-C with an IW612 (Murata 2EL) module and a
wired Ethernet backbone:

.. code-block:: console

   west build -p -b mimxrt1060_evk@C/mimxrt1062/qspi \
       -d build_mimxrt1060_evk/border_router_iw612_eth \
       --shield "nxp_m2_2ll_wifi_bt nxp_m2_2el_2ll_ieee802154" \
       zephyr/samples/net/openthread/border_router \
       -- -DEXTRA_CONF_FILE=" \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/mimxrt1060_evk_mimxrt1062_qspi_C.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/eth.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/iw612.conf"

For an IW610 (Murata 2LL) module, use ``iw610.conf`` instead:

.. code-block:: console

   west build -p -b mimxrt1060_evk@C/mimxrt1062/qspi \
       -d build_mimxrt1060_evk/border_router_iw610_eth \
       --shield "nxp_m2_2ll_wifi_bt nxp_m2_2el_2ll_ieee802154" \
       zephyr/samples/net/openthread/border_router \
       -- -DEXTRA_CONF_FILE=" \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/mimxrt1060_evk_mimxrt1062_qspi_C.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/eth.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/iw610.conf"

Because the Wi-Fi driver must stay enabled to load the M.2 firmware, a Wi-Fi
network interface is still present with ``eth.conf``.

.. note::
   The shield selects the HDLC RCP SPI interface and enables OpenThread L2 by
   default. The host SPI controller pinmux and GPIO assignments are provided by
   the board-specific overlay in ``boards/<board>.overlay``.

RCP Bring-up
************

The IW612/IW610 firmware, which also runs the IEEE 802.15.4 radio, is uploaded
over the Bluetooth H:4 UART by the NXP HCI driver. The RCP SPI interface is
built with :kconfig:option:`CONFIG_HDLC_RCP_IF_SPI_DEFERRED_INIT` and is started
by the same driver once the upload completes, so no application code is needed:

- :kconfig:option:`CONFIG_BT_NXP_CTRL_ENABLE_AT_BOOT` (enabled by default by
  the shield) calls ``bt_enable()`` at boot to upload the firmware.
- :kconfig:option:`CONFIG_BT_NXP_CTRL_HDLC_RCP_INIT` then drives the
  ``spi-enable-gpios`` line of the Bluetooth node, if any, initializes the HDLC
  RCP interface and brings the OpenThread interface up.

Applications that call ``bt_enable()`` themselves must disable
:kconfig:option:`CONFIG_BT_NXP_CTRL_ENABLE_AT_BOOT`.

Pin Assignments (MIMXRT1060-EVK Rev-C)
**************************************

On the MIMXRT1060-EVK Rev-C the RCP link uses LPSPI4 with the following signals:

.. list-table::
   :header-rows: 1
   :widths: 20 30 50

   * - Signal
     - Pin
     - Notes
   * - SPI SDI
     - GPIO_B0_01
     - LPSPI4 SDI
   * - SPI SDO
     - GPIO_B0_02
     - LPSPI4 SDO
   * - SPI SCK
     - GPIO_B0_03
     - LPSPI4 SCK
   * - SPI CS
     - GPIO_B0_00 (GPIO2.00)
     - Chip select, active low
   * - SPI INT
     - GPIO_B0_04 (GPIO2.04)
     - RCP host interrupt, active low
   * - SPI enable
     - PCAL6408A (LPI2C1, address 0x20) pin 0
     - Routes the RCP SPI lines to the M.2 connector, driven by the NXP HCI
       driver through ``spi-enable-gpios``

.. note::
   The board overlay disables the LCD interface (``&lcdif``) to free the
   GPIO_B0_xx pins used by LPSPI4.
