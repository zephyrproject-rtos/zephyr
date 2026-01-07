.. _snippet-video-sw-generator:

Video Software Generator Snippet (video-sw-generator)
#####################################################

.. code-block:: console

   west build -S video-sw-generator [...]

Overview
********

This snippet instantiate a fake video source generating a test pattern continuously
for test purpose. It is selected as the ``zephyr,camera`` :ref:`devicetree` chosen node.

Requirements
************

No hardware support is required besides sufficient memory for the video resolution
declared by :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE`.
