.. _external_module_zscilib:

Zephyr Scientific Library (zscilib)
###################################

Introduction
************

The Zephyr Scientific Library (`zscilib`_) is an attempt to provide a set of
functions useful for scientific computing, data analysis, and data manipulation
in the context of resource-constrained embedded hardware devices.

It is written entirely in C. While the main development target for the library
is the Zephyr Project, it aims to be as portable as possible. A standalone
reference project is included to use this library in non-Zephyr-based projects.

Usage with Zephyr
*****************

To pull in zscilib as a Zephyr module, either add it as a West project in the ``west.yaml``
file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/zscilib.yaml``) file
with the following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: zscilib-
         url: https://github.com/zephyrproject-rtos/zscilib
         revision: master
         path: modules/lib/zscilib # adjust the path as needed

For more detailed instructions and API documentation, refer to the `zscilib documentation`_ as
well as the provided `zscilib examples`_.


Running a sample application
============================

To run one of the sample applications using qemu, run the following commands:

.. code-block:: console

    $ west build -p -b qemu_cortex_a53 \
        samples/matrix/mult -t run
    ...
    *** Booting Zephyr OS build zephyr-v2.6.0-536-g89212a7fbf5f  ***
    zscilib matrix mult demo


    mtx multiply output (4x3 * 3x4 = 4x4):

    14.000000 17.000000 20.000000 23.000000
    35.000000 44.000000 53.000000 62.000000
    56.000000 71.000000 86.000000 101.000000
    7.000000 9.000000 11.000000 13.000000

Press CTRL+A then x to quit qemu.

Running Unit Tests
====================

To run the unit tests for this library, run the following command:

.. code-block:: console

    $ twister --inline-logs -p mps2/an521/cpu0 -T tests
    See the tests folder for further details.



Reference
*********

.. _zscilib:
    https://github.com/zephyrproject-rtos/zscilib

.. _zscilib documentation:
    https://zephyrproject-rtos.github.io/zscilib/

.. _zscilib examples:
    https://github.com/zephyrproject-rtos/zscilib/tree/master/samples
