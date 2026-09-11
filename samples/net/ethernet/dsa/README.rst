.. zephyr:code-sample:: dsa
   :name: DSA (Distributed Switch Architecture)
   :relevant-api: dsa_core

   Test and debug Distributed Switch Architecture

Overview
********

Example on testing/debugging Distributed Switch Architecture

The source code for this sample application can be found at:
:zephyr_file:`samples/net/ethernet/dsa`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

Follow these steps to build the DSA sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/ethernet/dsa
   :board: <board to use>
   :conf: prj.conf
   :goals: build
   :compact:

Using the Nucleo H755ZI-Q With the KSZ8463ML Evaluation Board
=============================================================

The KSZ8463ML evaluation board uses a MII connector for connecting the MAC of port 3
--- i.e. the host/CPU port --- to an external MAC. Using the evaluation board with
the STM32 Nucleo H755ZI-Q, which comes with an integrated LAN8742A PHY connected via
RMII, can therefore be done in one of two ways.

#. Desolder the LAN8742A PHY from the Nucleo board and connect the respective MII lines
   from the KSZ8463 directly to the extension connectors/through holes on the former
#. Hook the KSZ8463ML evaluation board up to an external PHY and connect said PHY
   to the Nucleo board via Ethernet (untested).

Note that the strap-in configuration differs between the two options. See Section 3.1.1
in the KSZ8463ML/RL user guide.
