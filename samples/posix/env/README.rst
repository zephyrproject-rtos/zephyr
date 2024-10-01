.. _posix-env-sample:

POSIX Environment Variables
###########################

Overview
********

In this sample application, the POSIX :c:func:`setenv`, function is used to populate several environment
variables in C. Then, all environment variables are then printed.

If the user sets a new value for the ``ALERT`` environment variable, it is printed to standard
output, and then cleared via :c:func:`unsetenv`.

Building and Running
********************

This project outputs to the console. It can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/posix/env
   :host-os: unix
   :board: qemu_riscv32
   :goals: run
   :compact:

Sample Output
=============

The program below shows sample output for a specific Zephyr build.

.. code-block:: console

    BOARD=qemu_riscv32
    BUILD_VERSION=zephyr-v3.5.0-5372-g3a46f2d052c7
    ALERT=

Setting Environment Variables
=============================

The shell command below shows how to create a new environment variable or update the value
associated with an existing environment variable.

The C code that is part of this sample application displays the value associated with the
``ALERT`` environment variable and then immediately unsets it.

.. code-block:: console

    uart:~$ posix env set ALERT="Happy Friday!"
    uart:~$ ALERT="Happy Friday!"
    uart:~$ posix env set HOME="127.0.0.1"
    uart:~$


Getting Environment Variables
=============================

The shell command below may be used to display the value associated with one environment variable.

.. code-block:: console

    uart:~$ posix env get BOARD
    qemu_riscv32

The shell command below may be used to display all environment variables and their associated
values.

.. code-block:: console

    uart:~$ posix env get
    BOARD=qemu_riscv32
    BUILD_VERSION=zephyr-v3.5.0-5372-g3a46f2d052c7
    ALERT=

Unsetting Environment Variables
===============================

The shell command below may be used to unset environment variables.

.. code-block:: console

    uart:~$ posix env unset BOARD
