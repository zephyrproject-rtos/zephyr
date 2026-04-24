I2C Role-Switch Echo Sample
###########################

Overview
********

This sample demonstrates transient I2C controller-mode transmission while the
device remains logically target-attached on the same I2C controller.

It is intended to validate I2C role-switch support on NXP MCXN947 FRDM boards
using Flexcomm9 I2C.

Two roles are supported:

* **Requester**
  Sends a payload to the responder and waits for the echoed response.

* **Responder**
  Receives a payload in target mode, transmits the same payload back using
  controller mode, and returns to target-ready steady state.

Requirements
************

* Two FRDM-MCXN947 boards
* Serial console access to both boards
* Flexcomm9 I2C wiring between boards
* Common ground between boards

Wiring
******

Connect the Flexcomm9 I2C signals between the two boards:

* requester SCL <-> responder SCL
* requester SDA <-> responder SDA
* requester GND <-> responder GND

If external pull-ups are required for your setup, add them according to the
board and bus electrical requirements.

Building
********

Build the responder image:

.. code-block:: console

   west build -b frdm_mcxn947/mcxn947/cpu0 samples/drivers/i2c/role_switch_echo -- -DCONF_FILE="prj.conf;prj_responder.conf"

Build the requester image:

.. code-block:: console

   west build -b frdm_mcxn947/mcxn947/cpu0 samples/drivers/i2c/role_switch_echo -- -DCONF_FILE="prj.conf;prj_requester.conf"

Flash each image to a different board.

Running
*******

1. Boot the responder board first.
2. Boot the requester board.
3. Observe UART logs on both boards.

Expected responder log flow
***************************

.. code-block:: text

   responder started: local=0x10 remote=0x11 payload=8
   target RX complete: len=8
   RX: 00 01 02 03 04 05 06 07
   controller TX complete to 0x11, pass=1

Expected requester log flow
***************************

.. code-block:: text

   requester started: local=0x11 remote=0x10 payload=8 count=100
   sending seq=0 to 0x10
   TX: 00 01 02 03 04 05 06 07
   PASS seq=0 total_pass=1
   RX: 00 01 02 03 04 05 06 07

Test purpose
************

This sample verifies:

* target-mode receive on Flexcomm9
* transient controller-mode transmit on the same controller
* return to target-ready steady state after transmit
* repeated role-switch operation across multiple transactions

Pass criteria
*************

The feature is considered working if:

* the requester successfully sends payloads to the responder
* the responder echoes them back correctly
* the requester reports PASS repeatedly
* the bus does not lock up across repeated transactions
* both boards continue running without reset or timeout
