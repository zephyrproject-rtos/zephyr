.. zephyr:code-sample:: fatfs-fstab
   :name: Fatfs filesystem fstab
   :relevant-api: file_system_api

   Define fatfs filesystems in the devicetree.

Overview
***********

This sample shows how to define a fatfs fstab entry in the devicetree.
This scenario uses a fatfs on a RAM disk. The sample is run on the
``native_sim`` board.

Building and running
********************

To run this sample, build it for the ``native_sim`` board
and afterwards run the generated executable file within the build folder.

The RAM disk sample for the ``native_sim`` board can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/fatfs_fstab
   :host-os: unix
   :board: native_sim
   :gen-args: -DDTC_OVERLAY_FILE=fatfs_fstab.overlay
   :goals: build
   :compact:

Sample Output
=============

When the sample runs successfully you should see following message on the screen:
.. code-block:: console

  I: Filesystem access successful
