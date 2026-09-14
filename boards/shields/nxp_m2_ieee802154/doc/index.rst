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

This single shield supports both M.2 modules:

- IW612 (Murata 2EL) module
- IW610 (Murata 2LL) module

The choice of module is made through the module-specific configuration fragment
passed on the command line (``mimxrt1060_evkc_iw612.conf`` for IW612 or
``mimxrt1060_evkc_iw610.conf`` for IW610). The shield overlay and pin
assignments are identical for both modules.

The OpenThread Border Router is a Wi-Fi + Thread application. It combines the
Wi-Fi/Bluetooth M.2 shield (``nxp_m2_2ll_wifi_bt``) with this 802.15.4 RCP
shield (``nxp_m2_2el_2ll_ieee802154``), and requires the RCP host overlay plus
the board-specific and module-specific configuration fragments.

To build for the MIMXRT1060-EVK Rev-C with an IW612 (Murata 2EL) module:

.. code-block:: console

   west build -p -b mimxrt1060_evk@C/mimxrt1062/qspi \
       -d build_mimxrt1060_evk/border_router_iw612 \
       --shield "nxp_m2_2ll_wifi_bt nxp_m2_2el_2ll_ieee802154" \
       zephyr/samples/net/openthread/border_router \
       -- -DEXTRA_CONF_FILE="overlay-ot-rcp-host-wifi-nxp.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/mimxrt1060_evk_mimxrt1062_qspi_C.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/mimxrt1060_evkc_iw612.conf"

To build for the MIMXRT1060-EVK Rev-C with an IW610 (Murata 2LL) module, use the
IW610 module fragment instead:

.. code-block:: console

   west build -p -b mimxrt1060_evk@C/mimxrt1062/qspi \
       -d build_mimxrt1060_evk/border_router_iw610 \
       --shield "nxp_m2_2ll_wifi_bt nxp_m2_2el_2ll_ieee802154" \
       zephyr/samples/net/openthread/border_router \
       -- -DEXTRA_CONF_FILE="overlay-ot-rcp-host-wifi-nxp.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/mimxrt1060_evk_mimxrt1062_qspi_C.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/mimxrt1060_evkc_iw610.conf"

The configuration fragments passed via ``EXTRA_CONF_FILE`` are applied in order:

- ``overlay-ot-rcp-host-wifi-nxp.conf``: base OpenThread RCP host + NXP Wi-Fi
  configuration (originally tuned for RW610).
- ``boards/mimxrt1060_evk_mimxrt1062_qspi_C/mimxrt1060_evk_mimxrt1062_qspi_C.conf``:
  RT1060 EVKC overrides for the M.2 IW612/IW610 RCP setup (disables RW610
  specific options, sizes heap and network buffers, sets logging).
- ``boards/mimxrt1060_evk_mimxrt1062_qspi_C/mimxrt1060_evkc_iw612.conf`` (or
  ``mimxrt1060_evkc_iw610.conf``): module-specific settings selecting the IW612
  or IW610 module respectively.

.. note::
   The shield selects the HDLC RCP SPI interface and enables OpenThread L2 by
   default. The host SPI controller pinmux and GPIO assignments are provided by
   the board-specific overlay in ``boards/<board>.overlay``.

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

.. note::
   The board overlay disables the LCD interface (``&lcdif``) to free the
   GPIO_B0_xx pins used by LPSPI4.
