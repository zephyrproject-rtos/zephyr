MCTP SMBus/I2C Echo Sample
##########################

Overview
********

This sample validates the Zephyr MCTP SMBus/I2C target binding with two Zephyr
boards:

* **Requester**
  Sends MCTP messages over SMBus/I2C and waits for the echoed response.

* **Responder**
  Receives MCTP messages through the SMBus/I2C target binding and sends the
  same payload back to the requester.

The sample exercises libmctp integration, SMBus packet parsing, PEC
calculation, target-mode receive, and transient controller-mode transmit on
the same I2C peripheral.

Requirements
************

* Two boards with I2C target support and a driver capable of transient
  controller-mode transfers while target-attached
* Serial console access to both boards
* I2C SCL, SDA, and ground connected between boards

FRDM-MCXN947 overlays are provided for Flexcomm9 I2C. Other boards can run the
sample by providing an overlay with an ``mctp_i2c`` node compatible with
``zephyr,mctp-i2c-smbus-target``.

Wiring
******

Connect the I2C signals between the two boards:

* requester SCL <-> responder SCL
* requester SDA <-> responder SDA
* requester GND <-> responder GND

If external pull-ups are required for your setup, add them according to the
board and bus electrical requirements.

Building
********

Build the responder image:

.. code-block:: console

   west build -b frdm_mcxn947/mcxn947/cpu0 samples/subsys/pmci/mctp/i2c_smbus_echo -- -DCONF_FILE="prj.conf;prj_responder.conf" -DEXTRA_DTC_OVERLAY_FILE=boards/frdm_mcxn947_mcxn947_cpu0_responder.overlay

Build the requester image:

.. code-block:: console

   west build -b frdm_mcxn947/mcxn947/cpu0 samples/subsys/pmci/mctp/i2c_smbus_echo -- -DCONF_FILE="prj.conf;prj_requester.conf" -DEXTRA_DTC_OVERLAY_FILE=boards/frdm_mcxn947_mcxn947_cpu0_requester.overlay

Flash each image to a different board.

Running
*******

1. Boot the responder board first.
2. Boot the requester board.
3. Observe UART logs on both boards.

The requester sends two payload patterns, ``increment`` and ``xor``. Each
message is delivered through libmctp, echoed by the responder, and compared
against the expected payload on the requester.

Expected responder log flow
***************************

.. code-block:: text

   MCTP SMBus binding started: local-eid=8 local-i2c=0x10 remote-i2c=0x11
   MCTP responder started: local-eid=8
   MCTP RX from EID 20 len=8
   MCTP echoed message 1 to EID 20 len=8

Expected requester log flow
***************************

.. code-block:: text

   MCTP SMBus binding started: local-eid=20 local-i2c=0x11 remote-i2c=0x10
   MCTP requester started: local-eid=20 remote-eid=8 payload=8 count=100
   MCTP PASS pattern=increment seq=0
   MCTP pattern done: increment pass=100 fail=0
   MCTP pattern done: xor pass=100 fail=0
   MCTP summary: total_pass=200 total_fail=0

Test Purpose
************

This sample verifies:

* devicetree instantiation of the MCTP SMBus/I2C binding
* ``MCTP_I2C_SMBUS_TARGET_DT_DEFINE()``
* ``mctp_init()``, ``mctp_register_bus()``, and binding ``start()``
* target-mode receive of SMBus block-write MCTP frames
* SMBus PEC validation through the Zephyr CRC subsystem
* receive delivery into libmctp through ``mctp_bus_rx()``
* RX packet-buffer lifetime handling across repeated transactions
* transmit through ``mctp_message_tx()``
* transient controller-mode transmit on the same I2C controller

Pass Criteria
*************

The feature is considered working if:

* the requester reports MCTP PASS repeatedly
* the responder logs received and echoed MCTP messages
* both boards continue running without bus lockup, reset, or timeout
