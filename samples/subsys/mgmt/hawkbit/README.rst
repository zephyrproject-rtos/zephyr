.. zephyr:code-sample:: hawkbit-api
   :name: Eclipse hawkBit Direct Device Integration API
   :relevant-api: hawkbit json

   Update a device using Eclipse hawkBit DDI API.

Overview
********

The Eclipse hawkBit update server provides REST resources which are consumed by the
device to retrieve software update tasks. This API is based on HTTP standards
and a polling mechanism.

This sample shows how to use hawkBit DDI API in both a polling and manual
update mode.

Polling mode run automatically on a predefined period, probing the server
for updates and installing them without requiring user intervention. You can
access the sample source code for this hawkbit_polling.

Manual mode requires the user to call the server probe and then, if there is
an available update, it will install the update. You can access the sample
source code for this mode hawkbit_manual

Caveats
*******

* The Zephyr port of hawkBit is configured to run on a
  :zephyr:board:`STM32H573I-DK <stm32h573i_dk>` MCU by default. The application should
  build and run for other platforms with support internet connection. Some
  platforms need some modification. Overlay files would be needed to support
  Bluetooth LE, 6lowpan, 802.15.4 or OpenThread configurations as well as the
  understanding that most other connectivity options would require an edge
  gateway of some sort (Border Router, etc).

* The MCUboot bootloader is required for hawkBit to function properly.
  More information about the Device Firmware Upgrade subsystem and MCUboot
  can be found in :ref:`mcuboot`.

Building and Running
********************

The below steps describe how to build and run the hawkBit sample in
Zephyr. Where examples are given, it is assumed the sample is being build for
the :zephyr:board:`STM32H573I-DK <stm32h573i_dk>` (``BOARD=stm32h573i_dk``).

Step 1: Start the hawkBit Docker
================================

By default, the hawkbit application is set to run on http at port:8088 for the
web ui and port:8080 for the DDI API.

.. code-block:: console

   git clone https://github.com/eclipse-hawkbit/hawkbit.git
   cd hawkbit/docker/postgres
   docker compose -fdocker-compose-monolith-dbinit-with-ui-postgres.yml up -d

This will start the hawkbit server on the host system.

Step 2: Access the hawkBit UI and configure the server
======================================================

Open your browser to the server URL, ``<your-ip-address>:8088``, and
log into the server using ``admin`` as the login and password by default.

Before we continue we need to enable gateway security for the server and also
set the gateway security token.

For that, click on the Config in the left pane of UI.

In the ``authentication.gatewaytoken.key`` field, set it to ``abcd1234``, it is has to
be the same value as :kconfig:option:`CONFIG_HAWKBIT_DDI_SECURITY_TOKEN`. If you
are using :kconfig:option:`CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME`, the token can
be set at runtime, make sure to set the same value as you set in the server.

Also make sure to enable ``authentication.gatewaytoken.enabled``
to enable the gateway token authentication.

Step 3: Build hawkBit sample and mcuboot
========================================

hawkBit can be built for the :zephyr:board:`STM32H573I-DK <stm32h573i_dk>` as follows:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/subsys/mgmt/hawkbit
   :board: stm32h573i_dk
   :goals: build
   :west-args: --sysbuild
   :compact:

If you want to build it with the ability to set the hawkBit server address
and port during runtime enabling the
:kconfig:option:`CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME` option in the
``prj.conf`` file.

.. _hawkbit_sample_sign:

The firmware will be signed automatically by the build system with the
``root-rsa-2048.pem`` key. The key is located in the MCUboot repository.

Step 4: Flash both the bootloader and the sample application
============================================================

when sysbuild was used to build both the bootloader and the sample
application flashing both images is as simple as running the following command:

.. code-block:: console

   west flash

Once the image is flashed and booted, the sample will print the image build
time to the console. After it connects to the internet, in hawkbit server UI,
you should see the ``stm32h573i_dk`` show up in the Targets pane. It's time to
upload a firmware binary to the server, and update it using this UI.

Step 5: Building and signing the test image
===========================================

This time you need the file :file:`zephyr.signed.bin` from the build directory.

Go to the hawkBit UI and click on Software Modules in the left pane of UI. Click on
the ``+`` icon to add a new software module. Set the type to ``Application`` and fill
in the name and version fields, these values can be arbitrary. Before clicking on the
``Create`` button, make sure to also enable the option
``Create single software module distribution set``.
Then select ``App(s) only`` as the ``Distribution Set Type``.
This way hawkBit will create a distribution set with the created software module.
For zephyr there is a 1:1 relation between software module and distribution set.
After clicking on the ``Create`` button, you will be taken to a dialog to upload
the artifacts, here upload the :file:`zephyr.signed.bin` file.
Then click on the ``Finish`` button.

If you go to ``Targets`` in the left pane of UI and select a target, its name
should start with the boards name. Then click on the clip symbol to assign a
distribution set to the target. Select the distribution you just created and
click on the ``Assign`` button.

Step 6: Run the update
======================

Back in the terminal session that you used for debugging the board, either wait until the
hawkbit client checks for updates automatically (by default every 5 minutes),
reboot the board, or type the following command:

.. code-block:: console

   hawkbit run

And then wait. The board will ping the server, check if there are any new
updates, and then download the update you've just created. If everything goes
fine the message ``Update installed`` will be printed on the terminal.

Your board will reboot automatically and then start with the new image. After rebooting, the
board will print a different image build time then automatically ping the server
again and the message ``Image is already updated`` will be printed on the terminal.
