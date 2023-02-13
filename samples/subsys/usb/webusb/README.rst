.. _webusb-sample:

WebUSB sample application
#########################

For a deeper dive into the WebUSB, refer to
https://developers.google.com/web/updates/2016/03/access-usb-devices-on-the-web

WebUSB API Specification:
https://wicg.github.io/webusb/

Sample Overview
***************

This simple echo application demonstrates the WebUSB sample application.
This application receives the data and echoed back to the WebUSB
based web application (web page) running in the browser at host.
This application is intended for testing purposes only. For running
real usecase, implement applications based on the WebUSB API.
This sample can be found under :zephyr_file:`samples/subsys/usb/webusb` in the
Zephyr project tree.

Requirements
************

This project requires an USB device driver, which is available for multiple
boards supported in Zephyr.

Building and Running
********************

Build and flash webusb sample with:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/webusb
   :board: <board to use>
   :goals: flash
   :compact:

Testing with latest Google Chrome on host
*****************************************

This sample application requires latest Google Chrome, a web page
based on WebUSB API to connect to the USB device and optionally
http server running on localhost to serve the web page.

WebUSB is a powerful new feature added to the Web and it is available
only to secure origins. This means the web page/site that used to
connect to the device must be served over a secure connection (HTTPS).

Follow these steps to run the demo on your host system:

#. Run the latest Google Chrome on host.

#. Implement a web app (web page) using WebUSB API and run
   it on localhost.

   See the sample at https://github.com/finikorg/webusb-sample

   This sample web page demonstrate how to create and use a WebUSB
   interface, as well as demonstrate the communication between browser
   and WebUSB enabled device.

   #. To access JS app go to https://finikorg.github.io/webusb-sample/

   #. To host the demo page locally: Clone the repo and start a web server
      in the appropriate directory.

      .. code-block:: console

         $ python -m http.server

#. Connect the board to your host.

#. Once the device is booted, you should see a notification from
   Chrome: "Go to localhost to connect.". Click on the notification
   to open demo page.
   Note that at the moment WebUSB landing page notification is disabled
   in Chrome on Windows. See https://github.com/WICG/webusb#implementation-status

#. Click on Connect button to connect to the device.

#. Send some text to the device by clicking on the Send button. The demo app
   will receive the same text from the device and display it in the textarea.
