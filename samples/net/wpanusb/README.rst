.. _wpanusb-sample:

WPANUSB sample
##############

Overview
********

This application exports ieee802154 radio over USB to be used in
other OSes such as Linux.  For Linux, the ieee802154 stack would be
implemented using the Linux SoftMAC driver.

Requirements
************

- a Zephyr WPANUSB enabled board (such as the :ref:`nrf52840_pca10056`)
  connected via USB to a Linux host.
- WPANUSB kernel driver (in the process of being open sourced)
- IWPAN host tools at [http://wpan.cakelab.org/releases/](http://wpan.cakelab.org/releases/)

Building and Running
********************

There are configuration files for various setups in the
``samples/net/wpanusb`` directory:

- :file:`prj.conf`
    This is the standard default config.  This can be used by itself for
    hardware which has native 802.15.4 support.

- :file:`overlay-cc1200.conf`
    This overlay config enables support for CC1200

- :file:`overlay-cc2520.conf`
    This overlay config enables support for CC2520

- :file:`overlay-mcr20a.conf`
    This overlay config enables support for MCR20A

Build the WPANUSB sample like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/wpanusb
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Example building for the Nordic nRF52840 Development Kit:

.. zephyr-app-commands::
   :zephyr-app: samples/net/wpanusb
   :host-os: unix
   :board: nrf52840_pca10056
   :goals: run
   :compact:

Example building for the Quark SE C1000 Devboard with TI CC2520 support:

.. zephyr-app-commands::
   :zephyr-app: samples/net/wpanusb
   :host-os: unix
   :board: quark_se_c1000_devboard
   :conf: "prj.conf overlay-cc2520.conf"
   :goals: run
   :compact:

When connected to Linux with wpanusb kernel driver, it is recognized as:

.. code-block:: console

   ...
   T:  Bus=01 Lev=02 Prnt=02 Port=00 Cnt=01 Dev#=  3 Spd=12  MxCh= 0
   D:  Ver= 1.10 Cls=ff(vend.) Sub=00 Prot=00 MxPS=64 #Cfgs=  1
   P:  Vendor=2fe3 ProdID=0101 Rev=01.00
   C:  #Ifs= 1 Cfg#= 1 Atr=c0 MxPwr=100mA
   I:  If#= 0 Alt= 0 #EPs= 1 Cls=ff(vend.) Sub=00 Prot=00 Driver=wpanusb
   ...

The following script enables the network interface in Linux
(uses iwpan tool from above):

.. code-block:: console

    #!/bin/sh
    PHY=`iwpan phy | grep wpan_phy | cut -d' ' -f2`
    echo 'Using phy' $PHY
    iwpan dev wpan0 set pan_id 0xabcd
    iwpan dev wpan0 set short_addr 0xbeef
    iwpan phy $PHY set channel 0 26
    ip link add link wpan0 name lowpan0 type lowpan
    ip link set wpan0 up
    ip link set lowpan0 up
