.. zephyr:code-sample:: tagoio-http-post
   :name: TagoIO HTTP Post
   :relevant-api: bsd_sockets http_client dns_resolve tls_credentials

   Send random temperature values to TagoIO IoT Cloud Platform using HTTP.

Overview
********

This sample application implements an HTTP client that will do an HTTP post
request to `TagoIO`_ IoT Service Platform. The sample sends random temperature
values to simulate a real device. This can be used to speed-up development
and shows how to send simple JSON data to `TagoIO`_ servers.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/cloud/tagoio_http_post`.

Requirements
************

- A free `TagoIO`_ account
- A board with internet connectivity, see :ref:`networking`
- The example provides three ways to get internet:

  * Ethernet: Using default configuration
  * WiFi: Using default configuration plus wifi overlay

TagoIO Device Configuration
***************************

If you don't have a `TagoIO`_ account, simple create a free account at
`TagoIO`_.  After that, add a device selecting Custom HTTP(S) protocol.  That
is it! Now reveal your device token.  The token will be used to identify your
device when sending data.  You need fill ``CONFIG_TAGOIO_DEVICE_TOKEN`` at
:zephyr_file:`samples/net/cloud/tagoio_http_post/prj.conf` file with that
information.


Building and Running
********************

Ethernet
========

You can use this application on a supported board with ethernet port.  There
are many like :zephyr:board:`sam4e_xpro`, :zephyr:board:`sam_v71_xult`,
:zephyr:board:`frdm_k64f`, :zephyr:board:`nucleo_f767zi` etc.  Pick one and just build
tagoio-http-client sample application with minimal configuration:

.. zephyr-app-commands::
   :zephyr-app: samples/net/cloud/tagoio_http_post
   :board: [sam4e_xpro | sam_v71_xult/samv71q21 | frdm_k64f | nucleo_f767zi]
   :goals: build flash
   :compact:


WIFI
====

To enable WIFI support, you need a board with an embedded WIFI support like
:zephyr:board:`disco_l475_iot1` or you can add a shield like
:ref:`module_esp_8266` or :ref:`inventek_eswifi_shield`.  Additionally you
need fill ``CONFIG_TAGOIO_HTTP_WIFI_SSID`` with your wifi network SSID and
``CONFIG_TAGOIO_HTTP_WIFI_PSK`` with the correspondent password at
:zephyr_file:`samples/net/cloud/tagoio_http_post/overlay-wifi.conf` file.

.. zephyr-app-commands::
   :zephyr-app: samples/net/cloud/tagoio_http_post
   :board: disco_l475_iot1
   :gen-args: -DEXTRA_CONF_FILE=overlay-wifi.conf
   :goals: build flash
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/net/cloud/tagoio_http_post
   :board: [sam_v71_xult/samv71q21 | frdm_k64f | nucleo_f767zi]
   :shield: [esp_8266_arduino | inventek_eswifi_arduino_uart]
   :gen-args: -DEXTRA_CONF_FILE=overlay-wifi.conf
   :goals: build flash
   :compact:

Visualizing TagoIO dashboard
****************************

After you got some logs on console it is time to create a dashboard on the
TagoIO to visualize the data.

* Go to the TagoIO web console
* Create a dashboard as Normal, give it a denomination and move next
* Add a line plot graph. You will see your device, temperature variable will
  be automatically selected for you.
* Just Save and enjoy!

.. image:: img/TagoIO-pc.jpeg
     :width: 640px
     :align: center
     :alt: TagoIO web dashboard

You can experiment the TagoIO mobile application on your cellphone or tablet.
Simple go to your app store and search by TagoIO, install, sign in, enjoy!

.. image:: img/TagoIO-mobile.jpeg
     :width: 480px
     :align: center
     :alt: TagoIO mobile dashboard

More information at `TagoIO`_ and `TagoIO Documentation`_.

References
**********

.. target-notes::

.. _TagoIO:
   https://tago.io/

.. _TagoIO Documentation:
   https://docs.tago.io
