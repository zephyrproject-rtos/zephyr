.. _mr_mcxn_t1_uwb_shield:

MR-MCXN-T1-UWB shield
#####################

Overview
********

The MR-MCXN-T1-UWB (*UWB Node AMO*) is an expansion board for the
:zephyr:board:`mr_mcxn_t1` carrier. It carries an NXP SR250 ultra-wideband
subsystem and the glue needed to interface it with the 3V3 stacking header:

* **U1** — NXP SR250 UWB subsystem, 1V8 I/O domain, UCI over SPI
* **IC3** — P3A1604UKAZ 4-bit level translator for the SPI signals
* **IC4** — P3A1604UKAZ 4-bit level translator for the control signals
* **IC1** — AP2114HA-1.2 LDO deriving ``VDD_1V8`` from ``VDD_3V3``
* **J3** — 2x6 board-to-board connector to the carrier stacking header

Signal mapping
**************

The SR250's 1V8 signals reach the MCXN947 through the two translators and the
stacking header:

+----------------+----------------------+----------------------+
| SR250 (1V8)    | Header / SoC pin     | Function             |
+================+======================+======================+
| UWB_SPI_SCK    | P3_3 / FC7_P1        | LPSPI7 SCK           |
+----------------+----------------------+----------------------+
| UWB_SPI_MISO   | P3_2 / FC7_P0        | LPSPI7 SDO           |
+----------------+----------------------+----------------------+
| UWB_SPI_MOSI   | P3_4 / FC7_P2        | LPSPI7 SDI           |
+----------------+----------------------+----------------------+
| UWB_SPI_CS     | P3_5 / FC7_P3        | LPSPI7 PCS0          |
+----------------+----------------------+----------------------+
| UWB_WAKEUP     | P3_1 / FC6_P0        | GPIO3 1              |
+----------------+----------------------+----------------------+
| UWB_RSTN       | P3_6 / FC6_P1        | GPIO3 6              |
+----------------+----------------------+----------------------+
| UWB_HOST_IRQ   | P3_10 / FC6_P2       | GPIO3 10             |
+----------------+----------------------+----------------------+

Requirements
************

The SR250 runs mainline firmware downloaded by the host at startup. The
firmware blob ships with ``hal_nxp`` as a click-through binary blob; fetch it
once with:

.. code-block:: bash

   west blobs fetch hal_nxp

Programming
***********

Set ``--shield mr_mcxn_t1_uwb`` when building, for example:

.. code-block:: bash

   west build -b mr_mcxn_t1/mcxn947/cpu0 --shield mr_mcxn_t1_uwb \
       samples/uwb/ranging_controller

Run ``samples/uwb/ranging_controlee`` on the counterpart device for a
successful DS-TWR ranging session.
