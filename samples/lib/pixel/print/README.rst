.. zephyr:code-sample:: lib_pixel_print
   :name: Pixel Printiing Library

   Print images on the console.

Overview
********

A sample showcasing how to make use of the pixel library to visualize an image or histogram data
by printing it out on the console using `ANSI escape codes`_.

This allow interleaving debug logs with small previews of the image as a way to debug image data.

.. _ANSI escape codes: https://en.wikipedia.org/wiki/ANSI_escape_code

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/lib/pixel/print
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

To build for another board, change "native_sim" above to that board's name.

Sample Output
=============

.. code-block:: console
