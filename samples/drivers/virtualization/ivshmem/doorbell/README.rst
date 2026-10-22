.. zephyr:code-sample:: ivshmem-doorbell
   :name: IVSHMEM doorbell
   :relevant-api: ivshmem

   Use Inter-VM Shared Memory to exchange messages between two processes running on different
   operating systems.

Overview
********

This sample shows how two processes on different operating systems can
communicate using ivshmem. This is a subset of the functionality provided by
OpenAMP.

Prerequisites
*************

QEMU needs to available.

ivshmem-server needs to be available and running. The server is available in
Zephyr SDK or pre-built in some distributions. Otherwise, it is available in
QEMU source tree.

ivshmem-client needs to be available as it is employed in this sample as an
external application. The same conditions of ivshmem-server applies to the
ivshmem-server, as it is also available via QEMU.

Building and Running
********************

Building ivshmem-doorbell is as follows:

qemu_cortex_a53
===============

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/virtualization/ivshmem/doorbell
   :host-os: unix
   :board: qemu_cortex_a53
   :goals: run
   :compact:

qemu_kvm_arm64
==============

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/virtualization/ivshmem/doorbell
   :host-os: unix
   :board: qemu_kvm_arm64
   :goals: run
   :compact:

qemu_x86_64
===========

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/virtualization/ivshmem/doorbell
   :host-os: unix
   :board: qemu_x86_64
   :goals: run
   :compact:

How to
******

.. note::

   The ivshmem shared memory can be manipulated to crash QEMU and bring down
   Zephyr. Check :ref:`ivshmem_doorbell_sample_security` section for more details.

.. note::

   Due to limited RAM memory available in qemu_x86_64 dts, it is not possible
   to use the default shared memory size of ivshmem (4MB) for this platform.

Steps to reproduce this sample:

#. Run ivshmem-server. For the ivshmem-server, both number of vectors and
   shared memory size are decided at run-time (when the server is executed).
   For Zephyr, the number of vectors and shared memory size of ivshmem are
   decided at compile-time and run-time, respectively.

   - (Arm64) Use vectors == 2 for the project configuration in this sample.
     Here is an example:

     .. code-block:: console

        # n = number of vectors
        $ sudo ivshmem-server -n 2
        $ *** Example code, do not use in production ***

   - (x86_64) The default shared memory size is bigger than the memory
     available for x86_64. For the provided sample configuration:

     .. code-block:: console

        # n = number of vectors, l = shared memory size
        $ sudo ivshmem-server -n 2 -l 4096
        $ *** Example code, do not use in production ***

   - (Optional) If vectors != 2, you need to change ivshmem driver
     :kconfig:option:`CONFIG_IVSHMEM_MSI_X_VECTORS`.

#. Appropriately set ownership of :file:`/dev/shm/ivshmem` and
   ``/tmp/ivshmem_socket`` for your deployment scenario. For instance:

   .. code-block:: console

      # assumption: "ivshmem" group should be the only allowed to access ivshmem
      $ sudo chgrp ivshmem /dev/shm/ivshmem
      $ sudo chmod 060 /dev/shm/ivshmem
      $ sudo chgrp ivshmem /tmp/ivshmem_socket
      $ sudo chmod 060 /tmp/ivshmem_socket
      $

#. Run Zephyr.

   .. code-block:: console

      $ west build -t run
      -- west build: running target run
      [0/1] To exit from QEMU enter: 'CTRL+a, x'[QEMU] CPU: cortex-a53
      *** Booting Zephyr OS build zephyr-v3.3.0-1649-g612f49da5dee ***
      Use write_shared_memory.sh and ivshmem-client to send a message

#. Write a message in the shared memory. The shared memory size *must* be kept
   the same as specified for ivshmem-server. This is the purpose of the
   ``write_shared_memory`` script; failing to respect the shared memory size
   may lead to a QEMU crash. For instance:

   - (Arm64) a simple "hello world" message (the script assumes the default
     size of ivshmem-server):

     .. code-block:: console

        # ./write_shared_memory.sh -m "your message"
        $ ./write_shared_memory.sh -m "hello world"
        $

   - (x86_64) a simple "hello world" message:

     .. code-block:: console

        # ./write_shared_memory.sh -m "your message" -s <size of shared memory>
        # assumption: the user created ivshmem-server with size 4096
        $ ./write_shared_memory.sh -m "hello world" -s 4096
        $

5. Send an interrupt to the guest. Using ivshmem-client, for instance:

   .. code-block:: console

      # find out client id. In this execution, it is 0 (peer_id)
      $ ivshmem-client
      dump: dump peers (including us)
      int <peer> <vector>: notify one vector on a peer
      int <peer> all: notify all vectors of a peer
      int all: notify all vectors of all peers (excepting us)
      listen on server socket 3
      cmd> dump
      our_id = 1
      vector 0 is enabled (fd=7)
      vector 1 is enabled (fd=8)
      peer_id = 0
      vector 0 is enabled (fd=5)
      vector 1 is enabled (fd=6)
      cmd> int 0 0

#. The sample will print the text in the shared memory whenever an interrupt is
   received (in any of the ivshmem-vectors). Example of output for arm64:

   .. code-block:: console

      $ west build -t run
      -- west build: running target run
      [0/1] To exit from QEMU enter: 'CTRL+a, x'[QEMU] CPU: cortex-a53
      *** Booting Zephyr OS build zephyr-v3.3.0-1649-g612f49da5dee ***
      Use write_shared_memory.sh and ivshmem-client to send a message
      received IRQ and full message: hello world

Known Issues
************

The guest application should be started before the host one, even though the
latter starts the communication. This is because it takes a while for the guest
to actually register the IRQ (needs to enable PCI, map PCI BARs, enable IRQ,
map callback). If the host is initialized first, the guest may lose the first
IRQ and the protocol will not work.

.. _ivshmem_doorbell_sample_security:

Security
********

This sample assumes that the shared memory region size is constant; therefore,
once the memory is set during PCI configuration, it should not be tampered
with. This is straight-forward if you are writing an application and uses
:c:func:`mmap`; however, using shell tools (like :command:`echo`) will treat
the shared memory as a file, and overwrite the shared memory size to the input
length.

One way to ensure proper consistency is: (i) restrict access to the shared
memory to trusted users; a rogue user with improper access can easily truncate
the memory size to zero, for example by using :command:`truncate`, and make QEMU
crash, as the application will attempt to read the initial, bigger, size; and
(ii) make sure writes always respect the shared memory region size.
