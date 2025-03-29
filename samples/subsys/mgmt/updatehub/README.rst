.. zephyr:code-sample:: updatehub-fota
   :name: UpdateHub embedded Firmware Over-The-Air (FOTA) update
   :relevant-api: updatehub

   Perform Firmware Over-The-Air (FOTA) updates using UpdateHub.

Overview
********

UpdateHub is an enterprise-grade solution which makes it simple to remotely
update all your embedded devices.  It handles all aspects related to sending
Firmware Over-the-Air (FOTA) updates with maximum security and efficiency,
while you focus on adding value to your product.  It is possible to read more
about at `docs.updatehub.io`_.

This sample shows how to use UpdateHub in both a polling and manual update
mode.

Polling mode runs automatically on a predefined period, probing the server
for updates and installing them without requiring user intervention.

Manual mode requires the user to call the server probe and then, if there is
an available update, also requires the user to decide if it is appropriate to
update now or later.

You can access the sample source code at
:zephyr_file:`samples/subsys/mgmt/updatehub/src/main.c`.

Caveats
*******

* The Zephyr port of ``UpdateHub`` was initially developed to run on a
  :zephyr:board:`Freedom-K64F <frdm_k64f>` kit using the ethernet connectivity.  The
  application should build and run for other platforms with same connectivity.

* The sample provides overlay files to enable other technologies like WIFI,
  802.15.4 or OpenThread.  These technologies depends on
  hardware resources and the correspondent overlay was designed to be generic
  instead full optimized.

* It is important understand that some wireless technologies may require a
  gateway or some sort of border router.  It is out of scope provide such
  configuration in details.

* The MCUboot bootloader is required for ``UpdateHub`` function properly.
  Before chose a platform to test, make sure that SoC and board have support
  to it.  UpdateHub currently uses two slots to perform the upgrade.  More
  information about Device Firmware Upgrade subsystem and MCUboot can be found
  in :ref:`mcuboot`.

* ``UpdateHub`` acts like a service on Zephyr. It is heavily dependent on
  Zephyr sub-systems and it uses CoAP over UDP.


Building and Running
********************

The below steps describe how to build and run the ``UpdateHub`` sample in
Zephyr.  Open a terminal ``terminal 1`` and navigate to your Zephyr project
directory.  This allows to construct and run everything from a common place.

.. code-block:: console

    ls
    bootloader  modules  tools  zephyr


Step 1: Build/Flash MCUboot
===========================

The MCUboot can be built following the instructions in the :ref:`mcuboot`
documentation page.  Flash the resulting image file using west on
``terminal 1``.

.. zephyr-app-commands::
    :app: bootloader/mcuboot/boot/zephyr
    :board: frdm_k64f
    :build-dir: mcuboot-frdm_k64f
    :goals: build flash


Step 2: Start the UpdateHub Server
==================================

Step 2.1: UpdateHub-CE (Community Edition)
------------------------------------------

The Zephyr sample application is configured by default to use the UpdateHub-CE
server edition.  This version implies you need run your own server.  The
UpdateHub-CE is distributed as a docker container and can be on your local
network or even installed on a service provider like Digital Ocean, Vultr etc.
To start using the UpdateHub-CE simple execute the docker command with the
following parameters on another terminal ``terminal 2``.

.. code-block:: console

    docker run -it -p 8080:8080 -p 5683:5683/udp --rm
      updatehub/updatehub-ce:latest

Step 2.2: UpdateHub Cloud
-------------------------

The UpdateHub Cloud is an enterprise-grade solution.  It provides almost same
resources than UpdateHub-CE with the DTLS as main diferential.  For more
details on how to use the UpdateHub Cloud please refer to the documentation on
`updatehub.io`_.  The UpdateHub Cloud has the option to use CoAPS/DTLS or not.
If you want to use the CoAPS/DTLS, simply add the ``overlay-dtls.conf`` when
building the sample.  You can use the provided certificate for test this
example or create your own.  The below procedure instruct how create a new
certificate using openssl on a Linux machine on terminal ``terminal 2``.

.. code-block:: console

    openssl genrsa -out privkey.pem 512
    openssl req -new -x509 -key privkey.pem -out servercert.pem

The ``servercert`` and ``privkey`` files must be embedded in the application
by ``certificates.h`` file.  The following procedure can be used to generated
the required ``der`` files:

.. code-block:: console

    openssl x509 -in servercert.pem -outform DER -out servercert.der
    openssl pkcs8 -topk8 -inform PEM -outform DER -nocrypt -in privkey.pem
      -out privkey.der


The ``der`` files should be placed on the sample source at certificates
directory.

.. note::

    When using UpdateHub Cloud server it is necessary update your own
    ``overlay-prj.conf`` with option :kconfig:option:`CONFIG_UPDATEHUB_CE` equal ``n``.


Step 3: Configure UpdateHub Sample
==================================

The updatehub have several Kconfig options that are necessary configure to
make it work or tune communication.

Set :kconfig:option:`CONFIG_UPDATEHUB_CE` select between UpdateHub edition.  The ``y``
value will select UpdateHub-CE otherwise ``n`` selects UpdateHub Cloud.

Set :kconfig:option:`CONFIG_UPDATEHUB_SERVER` with your local IP address that runs the
UpdateHub-CE server edition.  If your are using a service provider a DNS name
is a valid option too.  This option is only valid when using UpdateHub-CE.

Set :kconfig:option:`CONFIG_UPDATEHUB_POLL_INTERVAL` with the polling period of your
preference, remembering that the limit is between 0 and 43200 minutes
(30 days).  The default value is 1440 minutes (24h).

Set :kconfig:option:`CONFIG_UPDATEHUB_PRODUCT_UID` with your product ID.  When using
UpdateHub-CE the valid is available at ``overlay-prj.conf.example`` file.


Step 4: Build UpdateHub App
===========================

In order to correctly build UpdateHub the overlay files must be use correctly.
More information about overlay files in :ref:`important-build-vars`.

.. note::
    It is out of scope at this moment provide support for experimental
    features.  However, the configuration and use is similar to the start
    point indicated on the experimental network interface.

Step 4.1: Build for Ethernet
----------------------------

The ethernet depends only from base configuration.

.. zephyr-app-commands::
    :zephyr-app: samples/subsys/mgmt/updatehub
    :board: [ frdm_k64f | nucleo_f767zi ]
    :build-dir: app
    :gen-args: -DEXTRA_CONF_FILE=overlay-prj.conf
    :goals: build
    :compact:

Step 4.2: Build for WiFi
------------------------

For WiFi, it needs add ``overlay-wifi.conf``.  Here a shield provides WiFi
connectivity using, for instance, arduino headers.  See :ref:`module_esp_8266`
for details.

.. zephyr-app-commands::
    :zephyr-app: samples/subsys/mgmt/updatehub
    :board: [ frdm_k64f | nrf52840dk/nrf52840 | nucleo_f767zi ]
    :build-dir: app
    :gen-args: -DEXTRA_CONF_FILE="overlay-wifi.conf;overlay-prj.conf"
    :shield: esp_8266_arduino
    :goals: build
    :compact:

.. note::
    The board disco_l475_iot1 is not supported.  The es-WIFI driver currently
    doesn't support UDP.

Step 4.3: Build for IEEE 802.15.4 [experimental]
------------------------------------------------

For IEEE 802.15.4 needs add ``overlay-802154.conf``.  This requires two nodes:
one will be the host and the second one will be the device under test.  The
validation needs a Linux kernel >= 4.9 with all 6loWPAN support. It is out of scope
at this moment provide support since it is experimental.  The gateway was
tested with both native linux driver and ``atusb``.

.. zephyr-app-commands::
    :zephyr-app: samples/subsys/mgmt/updatehub
    :board: nrf52840dk/nrf52840
    :build-dir: app
    :gen-args: -DEXTRA_CONF_FILE="overlay-802154.conf;overlay-prj.conf"
    :goals: build
    :compact:

.. zephyr-app-commands::
    :zephyr-app: samples/subsys/mgmt/updatehub
    :board: [ frdm_k64f | nucleo_f767zi ]
    :build-dir: app
    :gen-args: -DEXTRA_CONF_FILE="overlay-802154.conf;overlay-prj.conf"
    :shield: atmel_rf2xx_arduino
    :goals: build
    :compact:

Step 4.4: Build for OpenThread Network [experimental]
-----------------------------------------------------

The OpenThread requires the ``overlay-ot.conf``.  It requires two nodes:
one will be the host NCP and the second one will be the device under test.  The
validation needs a Linux kernel >= 4.9 with optional NAT-64 support.  The
start point is try reproduce the `OpenThread Router`_. It is
out of scope at this moment provide support since it is experimental.  The
gateway was tested using two boards with OpenThread 1.1.1 on NCP mode.

.. zephyr-app-commands::
    :zephyr-app: samples/subsys/mgmt/updatehub
    :board: nrf52840dk/nrf52840
    :build-dir: app
    :gen-args: -DEXTRA_CONF_FILE="overlay-ot.conf;overlay-prj.conf"
    :goals: build
    :compact:


Step 5: Sign the app image
==========================

The app image is the application itself that will be on the board.  This app
will connect to UpdateHub server and check for new images.  The image will be
loaded on the board with version 1.0.0.  It is important check what file
format you SoC tools uses.  In general, Zephyr can create images with binary
(``.bin``) image format or Intel's (``.hex``) image format.

The Zephyr provide the ``west`` tool that simplify the signing process.  Just
call west with proper parameter values:

.. code-block:: console

  west sign -t imgtool -d build/app -- --version 1.0.0 --pad
    --key bootloader/mcuboot/root-rsa-2048.pem

  === image configuration:
  partition offset: 131072 (0x20000)
  partition size: 393216 (0x60000)
  rom start offset: 512 (0x200)
  === signing binaries
  unsigned bin: <zephyrdir>/build/app/zephyr/zephyr.bin
  signed bin:   <zephyrdir>/build/app/zephyr/zephyr.signed.bin


Step 6: Flash the app image
===========================

.. code-block:: console

    west flash -d build/app --bin-file build/app/zephyr/zephyr.signed.bin

.. note:: Command variation to flash a ``hex`` file:
    ``west flash -d build/app --hex-file build/app/zephyr/zephyr.signed.hex``

At this point you can access a third terminal ``terminal 3`` to check if image
is running.  Open the ``terminal 3`` and press reset on your board:

.. code-block:: console

    minicom -D /dev/ttyACM0


Step 7: Signing the binary test image
=====================================

The test image needs different parameters to add the signature.  Pay attention
to make sure you are creating the right signed image.  The test image will be
created with version 2.0.0 in this tutorial:

.. code-block:: console

  west sign --no-hex --bin -B build/zephyr-2.0.0.bin -t imgtool -d build/app --
    --version 2.0.0 --key bootloader/mcuboot/root-rsa-2048.pem

  === image configuration:
  partition offset: 131072 (0x20000)
  partition size: 393216 (0x60000)
  rom start offset: 512 (0x200)
  === signing binaries
  unsigned bin: <zephyrdir>/build/app/zephyr/zephyr.bin
  signed bin:   build/zephyr-2.0.0.bin


Step 8: Create a package with UpdateHub Utilities (uhu)
=======================================================

First, install UpdateHub Utilities (``uhu``) on your system, using:

.. code-block:: console

    pip3 install --user uhu

After installing uhu you will need to set the ``product-uid``.  The value for
UpdateHub-CE can be found at ``overlay-prj.conf.example`` file.  For UpdateHub
Cloud, you need copy the value from the web interface.

.. code-block:: console

    uhu product use "e4d37cfe6ec48a2d069cc0bbb8b078677e9a0d8df3a027c4d8ea131130c4265f"

Then, add the package and its mode (``zephyr``):

.. code-block:: console

    uhu package add build/zephyr-2.0.0.bin -m zephyr

Then inform what ``version`` this image is:

.. code-block:: console

   uhu package version 2.0.0

And finally you can build the package by running:

.. code-block:: console

    uhu package archive --output build/zephyr-2.0.0.pkg

The remaining steps are dedicated to UpdateHub-CE.  If you are using UpdateHub
Cloud you can find the proper procedure at `docs.updatehub.io`_.


Step 9: Add the package to server
=================================

Now, add the package to the updatehub server.  Open your browser to the server
URL, ``<your-ip-address>:8080``, and logging into the server using ``admin``
as the login and password by default.  After logging in, click on the package
menu, then ``UPLOAD PACKAGE``, and select the package built in step 8.


Step 10: Register device on server
==================================

If you chose ``Manual``, register your device at updatehub server by using the
terminal session where you are debugging the board ``terminal 3``. Type the
following command:

.. code-block:: console

    updatehub run

If everything is alright, it will print on the screen ``No update available``.

For ``Polling`` mode, the system will automatically register your device after
:kconfig:option:`CONFIG_UPDATEHUB_POLL_INTERVAL` minutes.  The ``updatehub run`` can
be used to speed-up.

.. note::
    The message ``Could not receive data`` means that the application was not
    able to reached the updatehub server for some reason.  The most common
    cases are server down, missing network routes and forget to change the
    content of ``overlay-prj.conf`` file.


Step 11: Create a rollout
=========================

In the browser where the UpdateHub-CE is open, click on ``menu Rollout``
and then ``CREATE ROLLOUT``.  Select the version of the package that you added
in step 9.  With that, the update is published, and the server is ready to
accept update requests.


Step 12: Run the update
=======================

Back in the terminal session that you used for debugging the board, type the
following command:

.. code-block:: console

    updatehub run

And then wait.  The board will probe the server, check if there are any new
updates, and then download the update package you've just created. If
everything goes fine the message ``Image flashed successfully, you can reboot
now`` will be printed on the terminal.  If you are using the ``Polling`` mode
the board will reboot automatically and Step 13 can be skipped.


Step 13: Reboot the system
==========================

In the terminal you used for debugging the board, type the following command:

.. code-block:: console

    kernel reboot cold

Your board will reboot and then start with the new image.  After rebooting,
the board will automatically ping the server again and the message ``No update
available`` will be printed on the terminal.  You can check the newer version
using the following command:

.. code-block:: console

    uart:~$ updatehub info
    Unique device id: acbdef0123456789
    Firmware Version: 2.0.0
    Product uid: e4d37cfe6ec48a2d069cc0bbb8b078677e9a0d8df3a027c4d8ea131130c4265f
    UpdateHub Server: <server ip/dns>
    uart:~$

Hardware
********

The below list of hardware have been used by UpdateHub team.


.. csv-table::
   :header: "ID", "Network Interface", "Shield / Device"
   :widths: 5, 45, 50
   :width: 800px

   1, Ethernet, Native
   2, WIFI, :ref:`ESP-8266 <module_esp_8266>`
   3, "MODEM (PPP)", "SIMCOM 808"
   4, "IEEE 802.15.4 (6loWPAN)", "Native,
   :ref:`RF2XX <atmel_at86rf2xx_transceivers>`"
   6, "OpenThread Network", Native

.. csv-table::
   :header: "Board", "Network Interface"
   :widths: 50, 50
   :width: 800px

   :zephyr:board:`frdm_k64f`, "1, 2, 3, 4"
   :zephyr:board:`nrf52840dk`, "2, 3, 4, 5, 6"
   :zephyr:board:`nucleo_f767zi`, "1, 2, 3, 4"


.. _updatehub.io: https://updatehub.io
.. _docs.updatehub.io: https://docs.updatehub.io/
.. _OpenThread Router: https://openthread.io/guides/border-router
