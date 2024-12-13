.. _pmod_sd:

Digilent Pmod SD
################

Overview
********

The Pmod SD provides a full-sized SD card slot that can be accessed through SPI.

Programming
***********

Set ``--shield pmod_sd`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/disk/disk_access
   :board: ek_ra8m1
   :shield: pmod_sd
   :goals: build

References
**********

- `Pmod SD product page`_
- `Pmod SD reference manual`_
- `Pmod SD schematic`_

.. _Pmod SD product page:
   https://digilent.com/shop/pmod-sd-full-sized-sd-card-slot/

.. _Pmod SD reference manual:
   https://digilent.com/reference/pmod/pmodsd/reference-manual

.. _Pmod SD schematic:
   https://digilent.com/reference/_media/reference/pmod/pmodsd/pmodsd_sch.pdf
