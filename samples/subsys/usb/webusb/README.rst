.. zephyr:code-sample:: webusb
   :name: WebUSB
   :relevant-api: usbd_api

   Receive and echo data from a web page using WebUSB API.

For a deeper dive into the WebUSB, refer to
https://developers.google.com/web/updates/2016/03/access-usb-devices-on-the-web

WebUSB API Specification:
https://wicg.github.io/webusb/

Overview
********

This simple echo application demonstrates the WebUSB sample application.
This application receives the data and echoes back to the WebUSB
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

Testing with the latest Google Chrome on host
*********************************************

This sample application requires the latest Google Chrome, a web page
based on WebUSB API to connect to the USB device and optionally
http server running on localhost to serve the web page.

WebUSB is a powerful new feature added to the Web and it is available
only to secure origins. This means the web page/site that is used to
connect to the device must be served over a secure connection (HTTPS).

Follow these steps to run the demo on your host system:

#. Run the latest Google Chrome on host.

#. Implement a web app (web page) using WebUSB API and run
   it on localhost.

   The sample can be found in the webusb sample directory:
   :zephyr_file:`samples/subsys/usb/webusb/index.html`.

   This sample web page demonstrates how to create and use a WebUSB
   interface, as well as demonstrate the communication between browser
   and WebUSB enabled device.

   There are two ways to access this sample page:

   * Using Chrome browser go to :doc:`demo`

   * Host the demo page locally: Start a web server
     in the webusb sample directory.

     .. code-block:: console

        $ python -m http.server

     Using Chrome browser open url http://localhost:8001/

#. Connect the board to your host.

#. Once the device is booted, you should see a notification from
   Chrome: "Go to localhost to connect.". Click on the notification
   to open demo page.
   Note that at the moment WebUSB landing page notification is disabled
   in Chrome on Windows. See https://github.com/WICG/webusb#implementation-status

#. Click on the :guilabel:`Connect` button to connect to the device.

#. Send some text to the device by clicking on the :guilabel:`Send` button.
   The demo app will receive the same text from the device and display it in
   the text area.
