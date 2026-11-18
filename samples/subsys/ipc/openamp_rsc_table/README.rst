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

Shared memory
=============

Shared memory containing the resource table, vrings, and RPMsg buffers must
be non-cacheable unless both processors perform the required cache
maintenance. :kconfig:option:`CONFIG_OPENAMP_WITH_DCACHE` enables OpenAMP
vring cache operations for cacheable shared memory. The platform's cache
backend must support the shared-memory addresses, and any required cache
maintenance for the resource table and buffers must also be provided.

Starting the firmware
*********************

Copy the ELF from the selected build directory to Linux ``/lib/firmware``.
For the default ``build`` directory, run the following as root on Linux:

.. code-block:: console

   $ cp build/zephyr/zephyr_openamp_rsc_table.elf /lib/firmware/

Identify the remoteproc instance for the target core using its name and
device path in sysfs. Instance numbers are not fixed:

.. code-block:: console

   $ for rproc in /sys/class/remoteproc/remoteproc*; do
   >     printf '%s: ' "$rproc"
   >     cat "$rproc/name"
   >     readlink -f "$rproc/device"
   > done

Replace ``remoteprocX`` below with the selected instance. It must be
``offline`` before starting the firmware:

.. code-block:: console

   $ rproc=/sys/class/remoteproc/remoteprocX
   $ echo zephyr_openamp_rsc_table.elf > "$rproc/firmware"
   $ echo start > "$rproc/state"
   $ cat "$rproc/state"
   running

Refer to the board documentation for the supported core, build options,
and matching Linux configuration.

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

Open a Linux shell (minicom, ssh, etc.) and load the ``rpmsg_tty`` driver.
When :kconfig:option:`CONFIG_SHELL_BACKEND_RPMSG` is enabled with its default
``rpmsg-tty`` service name, it creates a second ``rpmsg-tty`` channel in
addition to the sample's TTY responder.

Use sysfs to match each TTY to the target remoteproc instance and RPMsg
endpoint rather than assuming a fixed ``ttyRPMSG`` number:

.. code-block:: console

   $ modprobe rpmsg_tty
   $ for tty in /sys/class/tty/ttyRPMSG*; do
   >     printf '/dev/%s: ' "$(basename "$tty")"
   >     readlink -f "$tty/device"
   > done

Replace ``ttyRPMSGX`` below with the sample's TTY responder, not the Shell
endpoint. Send a message to Zephyr, which answers with the
:samp:`TTY {<addr>}:` prefix. The address in the example is illustrative;
it corresponds to the Zephyr responder's RPMsg endpoint address:

.. code-block:: console

   $ tty=/dev/ttyRPMSGX
   $ stty -F "$tty" raw -echo
   $ cat "$tty" &
   $ echo "Hello Zephyr" > "$tty"
   TTY 0x0401: Hello Zephyr

For an interactive Shell, open the TTY corresponding to the RPMsg Shell
endpoint instead. Shell command output returns through that RPMsg TTY,
not the physical UART.

If small vrings cause Shell output to be truncated while transmit buffers
are temporarily full, enable
:kconfig:option:`CONFIG_SHELL_BACKEND_RPMSG_FORCE_TX_BLOCKING_MODE` to wait
for an available transmit buffer.

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
