.. _openAMP_rsc_table_sample:

OpenAMP Sample Application using resource table
###############################################

Overview
********

This application demonstrates how to use OpenAMP with Zephyr based on a resource
table. It is designed to respond to the `Linux rpmsg client sample <https://elixir.bootlin.com/linux/latest/source/samples/rpmsg/rpmsg_client_sample.c>`_.
This sample implementation is compatible with platforms that embed
a Linux kernel OS on the main processor and a Zephyr application on
the co-processor.

Building the application
*************************

Zephyr
-------

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/openamp_rsc_table
   :goals: test

Linux
------

Enable SAMPLE_RPMSG_CLIENT configuration to build and install
the rpmsg_client_sample.ko module on the target.

Running the sample
*******************

Zephyr console
---------------

Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board.

Linux console
---------------

Open a Linux shell (minicom, ssh, etc.) and insert a module into the Linux Kernel

.. code-block:: console

   root@linuxshell: insmod rpmsg_client_sample.ko

Result on Zephyr console
-------------------------

The following message will appear on the corresponding Zephyr console:

.. code-block:: console

   ***** Booting Zephyr OS v#.##.#-####-g########## *****
   Starting application thread!

   OpenAMP demo started
   Remote core received message 1: hello world!
   Remote core received message 2: hello world!
   Remote core received message 3: hello world!
   ...
   Remote core received message 100: hello world!
   OpenAMP demo ended.
