.. _hawkbit-api-sample:

Hawkbit Direct Device Integration API sample
############################################

Overview
********

The hawkbit update server provides REST resources which are consumed by the
device to retrieve software update tasks. This API is based on HTTP standards
and a polling mechanism.

This sample shows how to use Hawkbit DDI API in both a polling and manual
update mode.

Polling mode run automatically on a predefined period, probing the server
for updates and installing them without requiring user intervention. You can
access the sample source code for this hawkbit_polling.

Manual mode requires the user to call the server probe and then, if there is
an available update, it will install the update. You can access the sample
source code for this mode hawkbit_manual

Caveats
*******

* The Zephyr port of ``Hawkbit`` is configured to run on a
  :ref:`Freedom-K64F <frdm_k64f>` MCU by default. The application should
  build and run for other platforms with support internet connection. Some
  platforms need some modification. Overlay files would be needed to support
  BLE 6lowpan, 802.15.4 or OpenThread configurations as well as the
  understanding that most other connectivity options would require an edge
  gateway of some sort (Border Router, etc).

* The MCUboot bootloader is required for ``Hawkbit`` to function properly.
  More information about the Device Firmware Upgrade subsystem and MCUboot
  can be found in :ref:`mcuboot`.

Building and Running
********************

The below steps describe how to build and run the ``Hawkbit`` sample in
Zephyr. Where examples are given, it is assumed the sample is being build for
the Freedom-K64F Development Kit (``BOARD=frdm_k64f``).

Step 1: Build MCUboot
=====================

Build MCUboot by following the instructions in the :ref:`mcuboot` documentation
page.

Step 2: Flash MCUboot
=====================

Flash the resulting image file to the 0x0 address of the flash memory. This can
be done by,

.. code-block:: console

   west flash

Step 3: Start the Hawkbit Docker
================================

By default, the hawkbit application is set to run on http at port:8080.

.. code-block:: console

   sudo docker run -p 8080:8080 hawkbit/hawkbit-update-server:latest \
   --hawkbit.dmf.rabbitmq.enabled=false \
   --hawkbit.server.ddi.security.authentication.anonymous.enabled=true

This will start the hawkbit server on the host system.Opening your browser to
the server URL, ``<your-ip-address>:8080``, and logging into the server using
``admin`` as the login and password by default.

Step 4: Build Hawkbit
=====================

``Hawkbit`` can be built for the frdm_k64f as follows:

.. zephyr-app-commands::
    :zephyr-app: samples/net/hawkbit
    :board: frdm_k64f
    :conf: "prj.conf"
    :goals: build

.. _hawkbit_sample_sign:

Step 5: Sign and confirm the first image
========================================

From this section onwards you use a binary (``.bin``) image format.

.. code-block:: console

   west sign -t imgtool -- --key \
   ~/zephyrproject/bootloader/mcuboot/root-rsa-2048.pem --confirm \
   --version 1.0.0

The command above creates a signed and confirmed image file called
:file:`zephyr.signed.confirmed.bin` in the build directory. It's important for
the first image to be confirmed as MCUboot isn't able to confirm an image that
is flashed using a hardware tool, and Hawkbit will reboot to trigger a firmware
swap if it isn't able to confirm the running image on init.

Step 6: Flash the first image
=============================

Upload the :file:`zephyr.signed.confirmed.bin` file from Step 5 to image slot-0
of your board.

.. code-block:: console

   west flash --bin-file build/zephyr/zephyr.signed.confirmed.bin

Once the image is flashed and booted, the sample will print the image build
time to the console. After it connects to the internet, in hawkbit server UI,
you should see the the frdm_k64f show up in the Targets pane. It's time to
upload a firmware binary to the server, and update it using this UI.

Step 7: Building and signing the test image
===========================================

The easiest way to test the functionality of Hawkbit is to repeat step 4 to
build the sample again, so that the build time will be different. Then, similar
to step 5, sign the image and assign it a different version number but without
confirming it like so:

.. code-block:: console

   west sign -t imgtool -- --key \
   ~/zephyrproject/bootloader/mcuboot/root-rsa-2048.pem \
   --version 1.0.1

The command above creates a signed image file called
:file:`zephyr.signed.bin` in the build directory.

Upload the signed image to the server. Click Upload icon in left pane of UI and
create a new Software Module with type Apps (``name:hawkbit,version:1.0.1``).
Then upload the signed image to the server with Upload file Icon.

Click on distribution icon in the left pane of UI and create a new Distribution
with type Apps only (``name:frdm_k64f_update,version:1.0.1``). Assign the
``hawkbit`` software module to the created distribution. Click on Deployment
icon in the left pane of UI and assign the ``frdm_k64f_update`` distribution to
the target ``frdm_k64f``.

Step 8: Run the update
======================

Back in the terminal session that you used for debugging the board, type the
following command:

.. code-block:: console

   hawkbit run

And then wait. The board will ping the server, check if there are any new
updates, and then download the update you've just created. If everything goes
fine the message ``Image flashed successfully, you can reboot now`` will be
printed on the terminal.

Step 9: Reboot the system
=========================

In the terminal you used for debugging the board, type the following command:

.. code-block:: console

   kernel reboot cold

Your board will reboot and then start with the new image. After rebooting, the
board will print a different image build time then automatically ping the server
again and the message ``No update available`` will be printed on the terminal.

Step 10: Clone and build hawkbit with https
===========================================

Below steps clone and build the hawkbit with self-signed certificate
to support https.

.. code-block:: console

   git clone https://github.com/eclipse/hawkbit.git
   cd hawkbit/hawkbit-runtime/hawkbit-update-server/src/main/resources

* Generate the private key

.. code-block:: console

   openssl genrsa -des3 -out server.key 2048

* Generate the CSR

.. code-block:: console

   openssl req -new -key server.key -out server.csr

Once you run the command, it will prompt you to enter your Country,
State, City, Company name and enter the Command Name field with
``<your-ip-address>``.

* Generate the self-signed x509 certificate suitable to use on web server.

.. code-block:: console

   openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt

* Generate pem file from generated server.key and server.crt

.. code-block:: console

   cat server.key > server.pem
   cat server.crt >> server.pem

* Generate .pkcs12 file

.. code-block:: console

   openssl pkcs12 -export -in server.pem -out keystore.pkcs12

* Following command imports a .p12 into pkcs12 Java keystore

.. code-block:: console

   keytool -importkeystore -srckeystore keystore.pkcs12 -srcstoretype pkcs12 \
   -destkeystore hb-pass.jks -deststoretype pkcs12 \
   -alias 1 -deststorepass <password_of_p12>

* Edit the hawkbit application.properties file

.. code-block:: console

   vi application.properties

Change authentication security from false to true.

.. code-block:: console

   hawkbit.server.ddi.security.authentication.anonymous.enabled=true

* Enter the https details at last

.. code-block:: console

   server.hostname=localhost
   server.port=8443
   hawkbit.artifact.url.protocols.download-http.protocol=https
   hawkbit.artifact.url.protocols.download-http.port=8443

   security.require-ssl=true
   server.use-forward-headers=true

   server.ssl.key-store=  <hb-pass.jks file location>
   server.ssl.key-store-type=JKS
   server.ssl.key-password= <password_of_key>
   server.ssl.key-store-password= <password_of_key_store>

   server.ssl.protocol=TLS
   server.ssl.enabled-protocols=TLSv1.2
   server.ssl.ciphers=TLS_RSA_WITH_AES_256_CBC_SHA256,
                      TLS_RSA_WITH_AES_256_CBC_SHA

* Start Compile

.. code-block:: console

   cd ~/hawkbit

   mvn clean install -DskipTests=true

* Once the build is successful, run hawkbit

.. code-block:: console

   java -jar ./hawkbit-runtime/hawkbit-update-server/target/ \
        hawkbit-update-server-#version#-SNAPSHOT.jar

Step 11: Build Hawkbit HTTPS
============================

* Convert the server.pem file to self_sign.der and place the der file in
  hawkbit/src directory

``Hawkbit https`` can be built for the frdm_k64f as follows:

.. zephyr-app-commands::
    :zephyr-app: samples/net/hawkbit
    :board: frdm_k64f
    :conf: "prj.conf overlay-tls.conf"
    :goals: build

Repeat the steps from 5 to 9, to update the device over https.
