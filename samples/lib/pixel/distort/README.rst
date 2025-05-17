.. zephyr:code-sample:: lib_pixel_distort
   :name: Pixel Distortion Library

   Correct an input image distortion.

Overview
********

A sample showcasing how to make use of the pixel library to correct distortioni of an input image
by applying the inverse distortion producing an output of the desired size.

The input and output are printed as preview images on the console.

This sample corrects a fish-eye distortion effect for a particular lens used as exapmle.

The library uses an input grid as the definition of the lens to correct.

In addition to the C source of this grid, tuned for one particular lens, Python scripts are
used to regenerate this grid for any arbitrary lens.

The method used is the standard `OpenCV lens calibration`_ process, adapted the source format
of lib/pixel.

.. `OpenCV lens calibration`:: https://docs.opencv.org/4.x/dc/dbb/tutorial_py_calibration.html

Building and Running
********************

This application can be built and executed on the native simulator as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/lib/pixel/distort
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

To build for another board, change "native_sim" above to that board's name.

Sample Output
=============

.. code-block:: console
