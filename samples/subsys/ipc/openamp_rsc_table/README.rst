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

i.MX943 EVK
===========

The ``m33`` board qualifier selects Cortex-M33 Core1 (M33S), not the
Cortex-M33 Core0 running System Manager. Build the new core targets with:

.. code-block:: console

   west build -b imx943_evk/mimx94398/m7_1 -d build_m71 samples/subsys/ipc/openamp_rsc_table
   west build -b imx943_evk/mimx94398/m33 -d build_m33s samples/subsys/ipc/openamp_rsc_table

The per-core SoC DTSI files define the MU interrupts. The board's
``imx943_evk_a55_mcore_ipc.dtsi`` declares the shared-memory and resource-table
regions matching the Linux remoteproc layout. These regions are disabled by
default; the sample overlays enable only the selected core's regions, select
them through ``chosen``, and use MU channel 1 for both transmit and receive.
The SCMI mailbox is left unchanged.
The MU node labels identify the A55-to-M-core links independently of which
operating systems run on the processors. The ``_for_*`` suffix identifies the
processor accessing that endpoint; the original hardware node labels remain
available.

.. list-table:: i.MX943 IPC mappings (remote-core view)
   :header-rows: 1

   * - Core
     - MU node label
     - MU
     - IRQ
     - Shared-memory base / size
     - Resource-table base / size
   * - M7 Core0
     - ``mu_a55_m70_for_m70``
     - MU11_B, ``0x42740000``
     - 126
     - ``0x82000000`` / 32 MiB
     - ``0x82220000`` / 4 KiB
   * - M7 Core1
     - ``mu_a55_m71_for_m71``
     - MU13_A, ``0x42ad0000``
     - 128
     - ``0x84000000`` / 32 MiB
     - ``0x84220000`` / 4 KiB
   * - M33 Core1 (M33S)
     - ``mu_a55_m33s_for_m33s``
     - MU15_A, ``0x42b10000``
     - 283
     - ``0x20800000`` / 640 KiB
     - ``0x208a0000`` / 4 KiB

The SoC linker configuration retains the ``.resource_table`` section using the
common i.MX linker snippet. The sample copies the initial table to the shared
address selected by ``zephyr,ipc_rsc_table`` before initializing OpenAMP.

M33S uses 644 KiB of OCRAM in total, including the resource table. The M7
resource tables are located within their respective 32 MiB shared regions.
Both M7 configurations enable D-cache and map their respective 32 MiB shared
regions as non-cacheable MPU memory. Each mapping also covers its resource
table without allocating a separate MPU region.
The board DTSI describes both M33S shared OCRAM regions as non-cacheable MPU
memory. The Zephyr NXP XCache backend does not currently perform range
maintenance for the ``0x20800000`` OCRAM window, so cache maintenance alone
cannot make Linux updates visible there.
These non-cacheable MPU regions are enabled only when the corresponding
memory nodes are enabled. Other memory retains its normal MPU attributes.
The M7 Core1 and M33S configurations enable INFO logs for received messages.
They also use blocking RPMsg Shell transmission so that command output waits
for an available transmit buffer instead of being truncated when the small
vrings are temporarily full.

For M33S, the build renames the ELF vector-table section to ``.interrupts``
without changing its address or contents. The Linux ``imx_rproc`` driver
uses this section name to obtain the boot-vector address, ``0x0ffc0000``.
Without it, the driver falls back to address zero, which is not the M33S
vector-table location.

Copy the ELF from the corresponding build directory to Linux ``/lib/firmware``.
Use the remoteproc instance associated with ``imx943-cm71`` or ``imx943-cm33s``
to select and start that firmware; remoteproc instance numbers are not fixed.
Linux and System Manager must grant the selected core access to its MU and
shared memory.

Each running instance also enables the RPMsg Shell backend. It can therefore
create two ``rpmsg-tty`` channels: one for the Shell and one for the sample's
TTY responder. Match the Linux TTY devices to the remoteproc instance and
RPMsg endpoint using sysfs rather than assuming a fixed ``ttyRPMSG`` number.
Shell replies return through the RPMsg TTY device, not the physical UART.

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
