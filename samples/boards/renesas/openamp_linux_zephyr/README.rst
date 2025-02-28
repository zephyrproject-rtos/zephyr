.. zephyr:code-sample:: rz-openamp-linux-zephyr
   :name: OpenAMP Linux Zephyr RPMsg
   :relevant-api: mbox_interface

   Enable message exchange between two cores, with the application core running Linux
   and the real-time core running Zephyr, using the OpenAMP library.

Overview
********

This application demonstrates how to use OpenAMP for communication between Zephyr and Linux,
where both sides utilize the OpenAMP library.

On the Linux side, the application uses the OpenAMP library provided by `meta-openamp <https://github.com/OpenAMP/meta-openamp>`_, ensuring
a consistent and standardized approach to inter-core communication.

Building the application
*************************

Zephyr
======

Build command:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/renesas/openamp_linux_zephyr
   :board: rzg3s_smarc/r9a08g045s33gbg/cm33
   :goals: build
   :compact:

Running the sample
*************************

Linux setup
===========

The sample currently supports the Linux RZ/G Multi-OS Package V2.2.0. The build steps for Renesas Multi-OS Package V2.2.0 are described below.

1. Follow the procedures in ''3.Multi-OS Package Setup'' of `Release Note for RZ/G Multi-OS Package V2.2.0 <https://www.renesas.com/en/document/rln/release-note-rzg-multi-os-package-v220?r=1522841>`_
   without initiating the build process.
2. For RZ/G3S, add ``PLAT_M33_BOOT_SUPPORT=1`` by following this guide:
   `RZ/G3S SMARC Evaluation Board Kit <https://docs.zephyrproject.org/latest/boards/renesas/rzg3s_smarc/doc/index.html#programming-and-debugging>`_

3. Insert the highlighted lines of code into the following file: meta-rz-features/meta-rz-multi-os/meta-rzg3s/recipes-example/rpmsg-sample/files/platform_info.c

   .. code-block:: c
      :emphasize-lines: 5,6,7,8,9,10

      if (ret) {
      LPRINTF("failed rpmsg_init_vdev");
      goto err;
      }
      /* RPMsg virtio enables callback for avail flags */
      ret = virtqueue_enable_cb(rpmsg_vdev->rvq);
      if (ret) {
         LPRINTF("Failed release availability flags");
         goto err;
      }
      #ifndef __linux__ /* uC3 */
      start_ipi_task(rproc);
      #endif
      return rpmsg_virtio_get_rpmsg_device(rpmsg_vdev);

4. Start the build.

5. For deploying bootloader files, Linux kernel image, device tree and rootfs, follow the procedures in ''3.5.1 In case of configuring CA55 as Boot CPU'' of `Release Note for RZ/G Multi-OS Package V2.2.0 <https://www.renesas.com/en/document/rln/release-note-rzg-multi-os-package-v220?r=1522841>`_

Zephyr setup
============

1. Flash the sample to the board.

2. Open a serial terminal (minicom, putty, etc.) and connect to the board with the following
   settings:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

Linux console
=============

1. Open a Linux shell (minicom, ssh, etc.) and login as root:

   .. code-block:: console

      smarc-<device> login: root

2. Run the following command:

   .. code-block:: console

      root@smarc-<device>:~# rpmsg_sample_client 0 0

3. The following output should appear:

   .. code-block:: console

      root@smarc-<device>:~# rpmsg_sample_client 0 0
      [211] proc_id:0 rsc_id:0 mbx_id:0
      metal: info:      metal_uio_dev_open: No IRQ for device 10400000.mbox-uio.
      metal: info:      metal_uio_dev_open: No IRQ for device 11010000.cpg-uio.
      [211] Successfully probed IPI device
      metal: info:      metal_uio_dev_open: No IRQ for device 42f00000.rsctbl.
      [211] Successfully open uio device: 42f00000.rsctbl.
      [211] Successfully added memory device 42f00000.rsctbl.
      metal: info:      metal_uio_dev_open: No IRQ for device 43000000.vring-ctl0.
      [211] Successfully open uio device: 43000000.vring-ctl0.
      [211] Successfully added memory device 43000000.vring-ctl0.
      metal: info:      metal_uio_dev_open: No IRQ for device 43200000.vring-shm0.
      [211] Successfully open uio device: 43200000.vring-shm0.
      [211] Successfully added memory device 43200000.vring-shm0.
      metal: info:      metal_uio_dev_open: No IRQ for device 43100000.vring-ctl1.
      [211] Successfully open uio device: 43100000.vring-ctl1.
      [211] Successfully added memory device 43100000.vring-ctl1.
      metal: info:      metal_uio_dev_open: No IRQ for device 43500000.vring-shm1.
      [211] Successfully open uio device: 43500000.vring-shm1.
      [211] Successfully added memory device 43500000.vring-shm1.
      metal: info:      metal_uio_dev_open: No IRQ for device 42f01000.mhu-shm.
      [211] Successfully open uio device: 42f01000.mhu-shm.
      [211] Successfully added memory device 42f01000.mhu-shm.
      [211] Initialize remoteproc successfully.
      [211] proc_id:1 rsc_id:1 mbx_id:0
      [211] Initialize remoteproc successfully.
      [211] proc_id:0 rsc_id:0 mbx_id:1
      [211] Initialize remoteproc successfully.
      [211] proc_id:1 rsc_id:1 mbx_id:1
      [211] Initialize remoteproc successfully.
      [217] thread start
      [CM33] creating remoteproc virtio
      [CM33] initializing rpmsg shared buffer pool
      [CM33] initializing rpmsg vdev
      [CM33]  1 - Send data to remote core, retrieve the echo and validate its integrity ..
      [CM33] Remote proc init.
      [CM33] RPMSG endpoint has created. rp_ept:0xffff9c86f870
      [CM33] register sig:2 succeeded.
      [CM33] register sig:15 succeeded.
      [CM33] RPMSG service has created.
      [CM33] sending payload number 0 of size 17
      [CM33]  received payload number 0 of size 17
      [CM33] sending payload number 1 of size 18
      [CM33]  received payload number 1 of size 18

      [snip]

      [CM33] sending payload number 471 of size 488
      [CM33]  received payload number 471 of size 488
      [CM33] ************************************
      [CM33]  Test Results: Error count = 0
      [CM33] ************************************
      [CM33] Quitting application .. Echo test end
      [CM33] Stopping application...
      [211] 42f00000.rsctbl closed
      [211] 43000000.vring-ctl0 closed
      [211] 43200000.vring-shm0 closed
      [211] 43100000.vring-ctl1 closed
      [211] 43500000.vring-shm1 closed
      [211] 42f01000.mhu-shm closed

Zephyr console
==============

The following message will appear on the Zephyr console.

   .. code-block:: console

      *** Booting Zephyr OS build zephyr-v#.#.#-####-g########## ***
      I: Starting application..!
      I: Starting application threads!
      I: OpenAMP[remote]  linux responder demo started
      I: new_service_cb: message received from service rpmsg-service-0
      I: OpenAMP[remote] Linux sample client responder started
      I: OpenAMP demo ended
      I: OpenAMP Linux sample client responder ended
