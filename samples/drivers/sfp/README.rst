.. zephyr:code-sample:: sfp
   :name: SFP pluggable module
   :relevant-api: sfp_interface sff8472

   Read the identity and live diagnostics of an SFP/SFP+ module.

Overview
********

This sample reads the serial ID of the module in an SFP cage (vendor, part
and serial number, wavelength, bit rate) and then prints its digital
diagnostics (temperature, supply voltage, laser bias, transmit and receive
optical power, alarm and warning flags) three times, one second apart.

See :ref:`sfp_api` for the API and the devicetree binding.

Requirements
************

A board with an SFP cage described by an ``sff,sfp`` node, or one of the
emulated setups below. On ``native_sim`` and ``qemu_cortex_m3`` the board
overlay puts an emulated module on an emulated I2C bus, so no hardware is
needed.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/sfp
   :board: native_sim
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

   Module     : SFP/SFP+/SFP28, LC connector
   Vendor     : ACME PHOTONICS
   Part number: ACME-10G-SR rev A1
   Serial     : SN12345678 (date code 230115)
   Wavelength : 850 nm, 10300 MBd
   Temperature: 45.500 C
   Vcc        : 3.300 V
   TX bias    : 6.500 mA
   TX power   : 0.500 mW
   RX power   : 0.400 mW
   Flags      : alarms 0x211, warnings 0x008
   ...
   Done

To run it on a board, describe the cage in the board's devicetree or an
overlay:

.. code-block:: devicetree

   sfp0: sfp-cage {
           compatible = "sff,sfp";
           i2c-bus = <&i2c1>;
           mod-def0-gpios = <&gpio0 4 GPIO_ACTIVE_LOW>;
   };
