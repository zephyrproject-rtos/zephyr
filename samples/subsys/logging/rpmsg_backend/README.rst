.. zephyr:code-sample:: rpmsg_backend
   :name: OpenAMP using resource table with rpmsg logging backend
   :relevant-api: ipm_interface

   Send log messages using OpenAMP and a resource table.

Overview
********

This application demonstrates how to use OpenAMP with Zephyr based on a resource
table and then enable sending log messages over an rpmsg endpoint.
It is designed to work with:

* `Linux rpmsg tty driver <https://elixir.bootlin.com/linux/latest/source/drivers/tty/rpmsg_tty.c>`_

There is one quirk to be aware of: an rpmsg connection requires an initial
message from driver to device (Linux to Zephyr) to fully establish the endpoint.
Once that is done, the logging messages will flow from Zephyr to Linux.
This is easily done through an echo command to the rpmsg tty device.
More advanced solutions might involve udev rules or systemd services but
that is outside the scope of this sample and up to the Linux-side developers.

This sample implementation is compatible with platforms that embed
a Linux kernel OS on the main processor and a Zephyr application on
the co-processor.

Building the application
************************

Zephyr
======

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/logging/rpmsg_backend
   :goals: test

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

Open a Linux shell (minicom, ssh, etc.) and print the log messages coming through the rpmsg-tty endpoint created during the sample initialization.
Restated from above: The nature of rpmsg does require an initial handshake, so even though logging is
intended to be one-way from Zephyr to Linux, you need to send something from Linux to Zephyr first
to get things going. Once done, you can read from the devnode however you see fit (cat, read(), etc.).
You can do this by using a command like the following with output to follow:

.. code-block:: console

   $> echo '' > /dev/ttyRPMSG0
   $> cat /dev/ttyRPMSG0 &

   [00:00:09.001,000] <inf> rpmsg_backend: OpenAMP[remote] Hello Linux!

   [00:00:11.002,000] <inf> rpmsg_backend: OpenAMP[remote] Hello Linux!

   [00:00:13.002,000] <inf> rpmsg_backend: OpenAMP[remote] Hello Linux!

   [00:00:15.003,000] <inf> rpmsg_backend: OpenAMP[remote] Hello Linux!

Zephyr console
==============

On the Zephyr console, you'll see when the log is sent and the message itself

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v#.#.#-####-g########## ***
   [00:00:01.000,000] <inf> rpmsg_backend: Starting application!
   [00:00:01.000,000] <inf> rpmsg_backend: OpenAMP[remote] Linux rpmsg logger demo started
   Sending message...
   [00:00:03.000,000] <inf> rpmsg_backend: OpenAMP[remote] Hello Linux!
   Sending message...
   [00:00:05.001,000] <inf> rpmsg_backend: OpenAMP[remote] Hello Linux!
   Sending message...
   [00:00:07.001,000] <inf> rpmsg_backend: OpenAMP[remote] Hello Linux!
   Message received
   Sending message...
   [00:00:09.001,000] <inf> rpmsg_backend: OpenAMP[remote] Hello Linux!
   Sending message...
   [00:00:11.002,000] <inf> rpmsg_backend: OpenAMP[remote] Hello Linux!
   Sending message...
   [00:00:13.002,000] <inf> rpmsg_backend: OpenAMP[remote] Hello Linux!
   Sending message...
   [00:00:15.003,000] <inf> rpmsg_backend: OpenAMP[remote] Hello Linux!
