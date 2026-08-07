.. zephyr:code-sample:: openamp-rsc-table
   :name: OpenAMP using resource table
   :relevant-api: ipm_interface

   Send messages between two cores using OpenAMP and a resource table.

Overview
********

This application demonstrates how to use OpenAMP with Zephyr based on a resource
table. It is designed to respond to the:

* `Linux rpmsg client sample <https://elixir.bootlin.com/linux/latest/source/samples/rpmsg/rpmsg_client_sample.c>`_
* `Linux rpmsg tty driver <https://elixir.bootlin.com/linux/latest/source/drivers/tty/rpmsg_tty.c>`_

This sample implementation is compatible with platforms that embed
a Linux kernel OS on the main processor and a Zephyr application on
the co-processor.

Building the application
************************

Zephyr
======

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/openamp_rsc_table
   :goals: test

Running the client sample
*************************

Linux setup
===========

Enable ``SAMPLE_RPMSG_CLIENT`` configuration to build the :file:`rpmsg_client_sample.ko` module.

Zephyr setup
============

Open a serial terminal (minicom, putty, etc.) and connect to the board using default serial port settings.

Linux console
=============

Open a Linux shell (minicom, ssh, etc.) and insert the ``rpmsg_client_sample`` module into the Linux Kernel.
Right after, logs should be displayed to notify channel creation/destruction and incoming message.

.. code-block:: console

   root@linuxshell: insmod rpmsg_client_sample.ko
   [   44.625407] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: new channel: 0x401 -> 0x400!
   [   44.631401] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 1 (src: 0x400)
   [   44.640614] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 2 (src: 0x400)
   ...
   [   45.152269] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 99 (src: 0x400)
   [   45.157678] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: incoming msg 100 (src: 0x400)
   [   45.158822] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: goodbye!
   [   45.159741] virtio_rpmsg_bus virtio0: destroying channel rpmsg-client-sample addr 0x400
   [   45.160856] rpmsg_client_sample virtio0.rpmsg-client-sample.-1.1024: rpmsg sample client driver is removed


Zephyr console
==============

For each message received, its content is displayed as shown down below then sent back to Linux.

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v#.#.#-####-g########## ***
   Starting application threads!

   OpenAMP[remote] Linux responder demo started

   OpenAMP[remote] Linux sample client responder started

   OpenAMP[remote] Linux TTY responder started
   [Linux sample client] incoming msg 1: hello world!
   [Linux sample client] incoming msg 2: hello world!
   ...
   [Linux sample client] incoming msg 99: hello world!
   [Linux sample client] incoming msg 100: hello world!
   OpenAMP Linux sample client responder ended


Running the rpmsg TTY demo
**************************

Linux setup
===========

Enable ``RPMSG_TTY`` in the kernel configuration.

Zephyr setup
============

Open a serial terminal (minicom, putty, etc.) and connect to the board using default serial port settings.

Linux console
=============

Open a Linux shell (minicom, ssh, etc.) and print the messages coming through the rpmsg-tty endpoint created during the sample initialization.
On the Linux console, send a message to Zephyr which answers with the :samp:`TTY {<addr>}:` prefix.
<addr> corresponds to the Zephyr rpmsg-tty endpoint address:

.. code-block:: console

   $> cat /dev/ttyRPMSG0 &
   $> echo "Hello Zephyr" >/dev/ttyRPMSG0
   TTY 0x0401: Hello Zephyr

Zephyr console
==============

On the Zephyr console, the received message is displayed as shown below, then sent back to Linux.

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v#.#.#-####-g########## ***
   Starting application threads!

   OpenAMP[remote] Linux responder demo started

   OpenAMP[remote] Linux sample client responder started

   OpenAMP[remote] Linux TTY responder started
   [Linux TTY] incoming msg: Hello Zephyr
