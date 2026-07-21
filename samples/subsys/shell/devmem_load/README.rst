.. zephyr:code-sample:: shell-devmem-load
   :name: devmem load shell
   :relevant-api: shell_api

   Load data to device memory using devmem shell cmd.

Overview
********

This module adds a ``devmem load`` command that allows data to be loaded into
device memory. The ``devmem load`` command is supported by every transport the
Zephyr shell can run on.

After invoking the command in the Zephyr shell, the device receives the
transferred data and writes it to the specified memory address.
The transfer ends when the user presses :kbd:`Ctrl+X` followed by :kbd:`Ctrl+Q`.

Requirements
************

* A target configured with the shell interface, exposed through any of
  its :ref:`backends <backends>`.

Building and Running
********************

The sample can be built for several platforms.

Emulation Targets
=================

The sample may run on emulation targets. The following commands build the
application for the qemu_x86.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/devmem_load
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

After running the application, the console displays the shell interface, and
shows the shell prompt, at which point the user may type in the ``devmem load`` command.

.. note::

   When using the ``devmem load`` command over UART, it is recommended to use
   interrupts whenever possible.
   If this is not possible, reduce the baud rate to 9600.

   If you use polling mode, you should also use ``prj_poll.conf`` instead of
   ``prj.conf``.

Building for boards without UART interrupt support:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/devmem_load
   :host-os: unix
   :board: native_sim
   :gen-args: -DCONF_FILE=prj_poll.conf
   :goals: run
   :compact:

On-Hardware
===========

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/shell_module
   :host-os: unix
   :board: nrf52840dk/nrf52840
   :goals: flash
   :compact:

Sample Output
*************

After connecting to the UART console, you should see the following output:

.. code-block:: console

   uart:~$

The ``devmem load`` command can now be used:

.. code-block:: console

   uart:~$ devmem load 0x20020000
   Loading...
   press ctrl-x ctrl-q to escape

At this point, the ``devmem load`` command waits for data input.
You can either type the data directly from the console or send it from the host PC
(replace ``ttyX`` with your UART console device):

.. code-block:: bash

   xxd -p data > /dev/ttyX

.. note::

   It is important to use a plain-style hex dump.

Once the data transfer is complete, use :kbd:`Ctrl+X` followed by :kbd:`Ctrl+Q`
to quit the loader. The shell then displays the number of bytes read and returns
to the prompt:

.. code-block:: console

   Number of bytes read: 3442
   uart:~$

Options
========

The ``devmem load`` command currently supports the following option:

* ``-e``
  Interpret data as little endian, e.g. ``0xDEADBEFF`` → ``0xFFBEADDE``.
