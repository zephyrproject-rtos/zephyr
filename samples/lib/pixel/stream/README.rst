.. zephyr:code-sample:: lib_pixel_stream
   :name: Pixel Streaming Library

   Convert an input frame as a stream of lines.

Overview
********

A sample showcasing how to make use of the pixel library to convert its content without requiring
large intermediate buffers, but only line buffers.

The input and output will both be printed as preview images on the console.

Building and Running
********************

This application can be built and executed on the native simulator as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/lib/pixel/stream
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

To build for another board, change "native_sim" above to that board's name.

Sample Output
=============

.. code-block:: console
