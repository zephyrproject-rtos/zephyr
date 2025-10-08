.. _snippet-video-stm32-venc:

STM32 Video ENCoder (VENC) Snippet (video-stm32-venc)
#####################################################

.. code-block:: console

   west build -S video-stm32-venc [...]

Overview
********

This snippet instantiate the STM32 Video ENCoder (VENC) and set it
as ``zephyr,videoenc`` :ref:`devicetree` chosen node.

Requirements
************

The board must have the hardware support for the Video ENCoder (VENC).
