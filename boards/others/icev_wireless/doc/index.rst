.. zephyr:board:: icev_wireless

Overview
********

The ICE-V Wireless is a combined ESP32-C3 and iCE40 FPGA board.

See the `ICE-V Wireless Github Project`_ for details.

Hardware
********

This board combines an Espressif ESP32-C3-MINI-1 (which includes 4MB of flash in the module) with a
Lattice iCE40UP5k-SG48 FPGA to allow WiFi and Bluetooth control of the FPGA. ESP32 and FPGA I/O is
mostly uncommitted except for the pins used for SPI communication between ESP32 and FPGA. Several
of the ESP32C3 GPIO pins are available for additional interfaces such as serial, ADC, I2C, etc.

For details on ESP32-C3 hardware please refer to the following resources:

* `ESP32-C3-MINI-1 Datasheet`_
* `ESP32-C3 Datasheet`_
* `ESP32-C3 Technical Reference Manual`_

For details on iCE40 hardware please refer to the following resources:

* `iCE40 UltraPlus Family Datasheet`_

.. include:: ../../../espressif/common/soc-esp32c3-features.rst
   :start-after: espressif-soc-esp32c3-features

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The ICE-V Wireless provides 1 row of reference, ESP32-C3, and iCE40 signals
brought out to J3, as well as 3 PMOD connectors for interfacing directly to
the iCE40 FPGA. Note that several of the iCE40 pins brought out to the PMOD
connectors are capable of operating as differential pairs.

.. figure:: img/icev_wireless_back.jpg
   :align: center
   :alt: ICE-V Wireless (Back)

   ICE-V Wireless (Back)

The J3 pins are 4V, 3.3V, NRST, GPIO2, GPIO3, GPIO8, GPIO9, GPIO10, GPIO20,
GPIO21, FPGA_P34, and GND. Note that GPIO2 and GPIO3 may be configured for
ADC operation.

For PMOD details, please refer to the `PMOD Specification`_ and the image
below.

.. figure:: img/icev_wireless_pinout.jpg
   :align: center
   :alt: ICE-V Wireless Pinout

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`ICE-V Wireless Github Project`: https://github.com/ICE-V-Wireless/ICE-V-Wireless
.. _`ESP32-C3-MINI-1 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-c3-mini-1_datasheet_en.pdf
.. _`iCE40 UltraPlus Family Datasheet`: https://www.latticesemi.com/-/media/LatticeSemi/Documents/DataSheets/iCE/iCE40-UltraPlus-Family-Data-Sheet.ashx
.. _`PMOD Specification`: https://digilent.com/reference/_media/reference/pmod/pmod-interface-specification-1_2_0.pdf
