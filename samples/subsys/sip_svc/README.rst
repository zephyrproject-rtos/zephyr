.. _sip_svc_sample:

SiP SVC sample
##############

Overview
********

This sample demonstrates the usage of SiP SVC subsystem.This sample
performs a voltage value query from Secure Device Manager in Intel Agilex
SoC FPGA.

Caveats
*******

* SiP SVC subsystem relies on the firmware running in EL3 layer to be in compatible
  with protocol defined inside the SiP SVC subsystem used by the project.
* The sample application relies on the Trusted Firmware ARM firmware in
  intel Agilex SoC FPGA query the voltage levels in Secure Device Manager.

Building and Running
********************
For building the application:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/sip_svc
   :board: intel_socfpga_agilex_socdk
   :goals: build

For running the application the Zephyr image can be loaded in DDR memory
at address 0x00100000 and ATF BL31 at 0x00001000 from SD Card or QSPI Flash
in ATF BL2.

Sample Output
*************
.. code-block:: console

  *** Booting Zephyr OS build zephyr-v3.3.0-2963-gb5ba49ae300e ***
  Got response of transaction id 0x00 and voltage is 0.846878v
  Got response of transaction id 0x01 and voltage is 0.858170v
  Got response of transaction id 0x02 and voltage is 0.860168v
  Got response of transaction id 0x03 and voltage is 0.846832v
  Got response of transaction id 0x04 and voltage is 0.858337v
  Got response of transaction id 0x05 and voltage is 0.871704v
  Got response of transaction id 0x06 and voltage is 0.859421v
  Got response of transaction id 0x07 and voltage is 0.857254v
  Got response of transaction id 0x08 and voltage is 0.858429v
  Got response of transaction id 0x09 and voltage is 0.859879v
  Got response of transaction id 0x0a and voltage is 0.845139v
  Got response of transaction id 0x0b and voltage is 0.858459v
  Got response of transaction id 0x0c and voltage is 0.859283v
  Got response of transaction id 0x0d and voltage is 0.846207v
  Got response of transaction id 0x0e and voltage is 0.855850v
  Got response of transaction id 0x0f and voltage is 0.859283v
  Got response of transaction id 0x00 and voltage is 0.846832v
  Got response of transaction id 0x01 and voltage is 0.856049v
  Got response of transaction id 0x02 and voltage is 0.859879v
