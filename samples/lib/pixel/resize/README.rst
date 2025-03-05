.. zephyr:code-sample:: lib_pixel_resize
   :name: Pixel Resizing Library

   Resize an image using subsampling.

Overview
********

A sample showcasing how to make use of the pixel library to resize an input image to a smaller or
bigger output image, using the subsampling method. This helps generating a smaller preview of an
input image.

The input and output are printed as preview images on the console.

Building and Running
********************

This application can be built and executed on the native simulator as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/lib/pixel/resize
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

To build for another board, change "native_sim" above to that board's name.

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0-2611-gfaa7b74cfda7 ***
   [00:00:00.000,000] <inf> app: input image, 32x16, 1536 bytes:
   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄ shows-up ▄▄▄▄▄▄▄▄▄▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄ as color ▄▄▄▄▄▄▄▄▄▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄ on the   ▄▄▄▄▄▄▄▄▄▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄ terminal ▄▄▄▄▄▄▄▄▄▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄|
   [00:00:00.000,000] <inf> app: output image, bigger, 120x16, 7200 bytes:
   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄[...]▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄[...]▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄[...]▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄ shows-up ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄[...]▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄ as color ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄[...]▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄ on the   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄[...]▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄ terminal ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄[...]▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄[...]▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄[...]▄▄|
   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄[...]▄▄|
   [00:00:00.000,000] <inf> app: output image, smaller, 10x10, 300 bytes:
   ▄▄▄▄▄▄▄▄▄▄|
   ▄▄▄▄▄▄▄▄▄▄|
   ▄▄▄▄▄▄▄▄▄▄|
   ▄▄▄▄▄▄▄▄▄▄|
   ▄▄▄▄▄▄▄▄▄▄|

.. image:: preview.png
