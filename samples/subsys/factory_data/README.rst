.. _factory_data_subsys_sample:

Factory data sample
###################

Overview
********

This is a simple application demonstrating the implementation and usage of a custom factory data
backend.

Compared to the non-customized, flash backed backends, this one works with EEPROM, and is greatly
reduced in complexity (code size). To achive this, it does not fully obey the API specified by the
Doxygen documentation. This is meant to illustrate the flexibility of the factory data subsystem,
which allows for all kind of implementations as long as it follows the key-string-to-value logic.

The application itself stores some values in the factory data partition, then retreives them again.

Requirements
************

* A board with EEPROM support
* Or qemu_x86 target

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/factory_data` in
the Zephyr tree.

The sample can be build for several platforms, the following commands build the
application for the qemu_x86.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/factory_data
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

When running the image, the output on the console shows the factory data manipulation messages.

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.1.0-275-gcd94d331157e  ***

    *** Simplistic custom factory data implementation example ***

    Load all factory data entries

    Print all factory data entries
    - uuid: 00000000000000000000000000000000
    - mac_address: 00:00:00:00:00:00
    - value_max_len:
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000
      0000000000000000  0000000000000000

    Above values are booring? Use the factory data shell to manipulate them!
