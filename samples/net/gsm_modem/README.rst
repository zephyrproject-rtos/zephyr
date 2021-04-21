.. _gsm-modem-sample:

Generic GSM Modem Sample
########################

Overview
********

The Zephyr GSM modem sample application allows user to have a connection
to GPRS network. The connection to GSM modem is done using
:ref:`PPP (Point-to-Point Protocol) <ppp>`.

The source code of this sample application can be found at:
:zephyr_file:`samples/net/gsm_modem`.

Requirements
************

- GSM modem card. The sample has been tested with SIMCOM SIM808 card. All
  GSM modem cards should work as long as they support **AT+CGDCONT** command.
- Almost any Zephyr enabled board with sufficient resources can be used.
  The sample has been tested with :ref:`reel_board`.

Build and Running
*****************

If you are using an external modem like the SIMCOM card, then connect
the necessary pins like RX and TX from your Zephyr target board to the
modem card. Internal modems, like what is found in :ref:`particle_boron`
board, are not yet supported.

.. zephyr-app-commands::
   :zephyr-app: samples/net/gsm_modem
   :board: reel_board
   :goals: build flash
   :gen-args: -DCONFIG_MODEM_GSM_APN=\"internet\"
   :compact:

Note that you might need to change the APN name according to your GSM network
configuration.
