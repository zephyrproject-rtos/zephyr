.. zephyr:code-sample:: ext2-fstab
   :name: EXT2 filesystem fstab
   :relevant-api: file_system_api

   Define ext2 filesystems in the devicetree.

Overview
***********

This sample shows how to define an ext2 fstab entry in the devicetree.
The sample is run on the ``native_sim`` board and RAM disk is used for ext2 storage.

Building and running
********************

To run this sample, build it for the ``native_sim`` board
and afterwards run the generated executable file within the build folder.

The RAM disk sample for the ``native_sim`` board can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/ext2_fstab
   :host-os: unix
   :board: native_sim
   :gen-args: -DDTC_OVERLAY_FILE=ext2_fstab.overlay
   :goals: build
   :compact:

Sample Output
=============

When the sample runs successfully you should see following message on the screen:
.. code-block:: console

  I: Filesystem access successful
