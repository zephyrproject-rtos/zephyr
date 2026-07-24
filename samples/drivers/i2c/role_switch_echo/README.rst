I2C Role-Switch Echo Sample
###########################

Overview
********

This sample demonstrates transient I2C controller-mode transmission while a
device remains logically target-attached on the same I2C controller.

It is intended to validate I2C role-switch support with two Zephyr boards. The
provided overlay uses Flexcomm9 I2C on FRDM-MCXN947.

Two roles are supported:

* **Requester**
  Sends a payload to the responder and waits for the echoed response.

* **Responder**
  Receives a payload in target mode, transmits the same payload back using
  controller mode, and returns to target-ready steady state.

Requirements
************

* Two boards with I2C target support and a driver capable of transient
  controller-mode transfers while target-attached
* Serial console access to both boards
* I2C SCL, SDA, and ground connected between boards

FRDM-MCXN947 overlay support is provided. Other boards can run the sample by
providing an overlay that defines the ``role-switch-i2c`` devicetree alias.

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
   echoed packet 1 len=8

Expected requester log flow
***************************

.. code-block:: text

   requester started: local=0x11 remote=0x10 payload=8 count=100
   starting pattern: increment
   PASS pattern=increment seq=0
   pattern done: increment pass=100 fail=0
   starting pattern: xor
   PASS pattern=xor seq=0
   pattern done: xor pass=100 fail=0
   summary: total_pass=200 total_fail=0

Test purpose
************

This sample verifies:

* target-mode receive
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
