.. _snippet-video-jpegenc:

JPEGENC Video Encoder Snippet (video-jpegenc)
#############################################

.. code-block:: console

   west build -S video-jpegenc [...]

Overview
********

This snippet instantiates a software JPEG encoder based on the jpegenc
module, and selects it as the ``zephyr,videoenc`` :ref:`devicetree` chosen
node. It allows applications and tests using a JPEG encoder to run on any
platform, including :zephyr:board:`native_sim <native_sim>` and QEMU.

Requirements
************

The jpegenc module must be present in the west workspace.
