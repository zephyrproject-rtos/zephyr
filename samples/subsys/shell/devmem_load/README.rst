.. _shell-devmem-sample:

Load Device Memory
##################

This module add a ``devmem load`` command that allows data to be loaded into 
device memory.
The ``devmem load`` command is supported by every transport the shell can run 
on.

After using a command in the Zephyr shell, the device reads all transferred
data and writes to an address in the memory.
The transfer ends when the user presses :kbd:`CTRL+A` :kbd:`x`.

Usage
*****

Note: when using the devmem load command over UART it is recommended to use 
interrupts whenever possible.
If this is not possible, reduce the baud rate to 9600.

If you use poll you should also use ``prj_poll.conf`` instead of ``prj.conf``.

Building
********

The sample can be built for several platforms, the following commands build and
run the application with a shell for the ``frdm_k64f`` board.

.. zephyr-app-commands::
    :zephyr-app: samples/subsys/shell/devmem_load
    :board: frdm_k64f
    :goals: build flash
    :compact:

Building for boards without UART interrupt support:

.. zephyr-app-commands::
    :zephyr-app: samples/subsys/shell/devmem_load
    :board: quick_feather
    :gen-args: -DOVERLAY_CONFIG=prj_poll.conf
    :goals: build flash
    :compact:

Running
*******

After connecting to the UART console you should see the following output:

.. code-block:: console

    uart:~$

The ``devmem load`` command can now be used 
(``devmem load [option] [address]``):

.. code-block:: console

    uart:~$ devmem load 0x20020000
    Loading...
    press ctrl-x ctrl-q to escape


Now, the ``devmem load`` is waiting for data.
You can either type it directly from the console or send it from the host PC 
(replace ``ttyX`` with the appropriate one for your UART console):

.. code-block:: console

    xxd -p data > /dev/ttyX

(It is important to use plain-style hex dump)
Once the data is transferred, use :kbd:`CTRL+A` :kbd:`CTRL+Q` to quit the 
loader. It will print the number of the bytes read and return to the shell:

.. code-block:: console

    Number of bytes read: 3442
    uart:~$


Options
*******

Currently, the ``devmem load`` command supports the following argument:

- ``-e`` little endian parse e.g. ``0xDEADBEFF -> 0xFFBEADDE``
