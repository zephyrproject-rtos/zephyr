.. zephyr:code-sample:: video-shell
   :name: Video shell
   :relevant-api: video_interface

   Use the video API to operate video devices using Zephyr shell commands.


Description
***********

This sample application uses the :ref:`video_shell` to capture frames from a video capture
device then uses the :ref:`display_api` to display them onto an LCD screen (if any).


Requirements
************

This sample can make use of any video device, real or emulated, and can run on any platform capable
of running the Zephyr shell, including :zephyr:board:`native_sim`.


Building and Running
********************

For :zephyr:board:`native_sim`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/shell
   :board: native_sim
   :goals: build
   :compact:


Sample Output
=============

.. code-block:: console

   TODO


References
**********

.. target-notes::
