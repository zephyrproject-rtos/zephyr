.. _wpan_serial-sample:

802.15.4 "serial-radio" sample
##############################

Overview
********

The wpan_serial sample shows how to use hardware with 802.15.4 radio and USB
controller as a "serial-radio" device for Contiki-based border routers.

Requirements
************

The sample assumes that 802.15.4 radio and USB controller are supported on
a board.

Building and Running
********************

#. Before building and running this sample, be sure your Linux system's
   ModemManager is disabled, otherwise, it can interfere with serial
   port communication:

   .. code-block:: console

     $ sudo systemctl disable ModemManager.service

#. Build and flash the sample Zephyr application to a board
   with a 802.15.4 radio and USB controller
#. Connect board to Linux PC, /dev/ttyACM[number] should appear.
#. Run Contiki-based native border router (6lbr, native-router, etc)
   Example for Contiki:

   .. code-block:: console

     $ cd examples/ipv6/native-border-router
     $ make
     $ sudo ./border-router.native -v5 -s ttyACM0 fd01::1/64

Now you have a Contiki native board router.  You can access its web-based
interface with your browser using the server address printed in the
border-router output.

.. code-block:: console

  ...
  Server IPv6 addresses:
   0x62c5c0: =>fd01::212:4b00:531f:113a
  ...

Use your browser to access ``http://[fd01::212:4b00:531f:113a]/`` and you'll
see available neighbors and routes.
