.. _snippet-video-stm32-jpegenc:

STM32 Video JPEG encoder Snippet (video-stm32-jpegenc)
######################################################

.. code-block:: console

   west build -S video-stm32-jpegenc [...]

Overview
********

This snippet instantiate the STM32 JPEG encoder and set it
as ``zephyr,videoenc`` :ref:`devicetree` chosen node.

Requirements
************

The board must have the hardware support for the JPEG encoder.
