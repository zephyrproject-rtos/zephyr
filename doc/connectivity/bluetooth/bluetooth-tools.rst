.. _bluetooth-tools:

Bluetooth tools
###############

This page lists and describes tools that can be used to assist during Bluetooth
stack or application development in order to help, simplify and speed up the
development process.

.. _bluetooth-mobile-apps:

Mobile applications
*******************

It is often useful to make use of existing mobile applications to interact with
hardware running Zephyr, to test functionality without having to write any
additional code or requiring extra hardware.

The recommended mobile applications for interacting with Zephyr are:

* Android:

  * `nRF Connect for Android`_
  * `nRF Mesh for Android`_
  * `LightBlue for Android`_

* iOS:

  * `nRF Connect for iOS`_
  * `nRF Mesh for iOS`_
  * `LightBlue for iOS`_

.. _bluetooth_bluez:

Using BlueZ with Zephyr
***********************

The Linux Bluetooth Protocol Stack, BlueZ, comes with a very useful set of
tools that can be used to debug and interact with Zephyr's BLE Host and
Controller. In order to benefit from these tools you will need to make sure
that you are running a recent version of the Linux Kernel and BlueZ:

* Linux Kernel 4.10+
* BlueZ 4.45+

Additionally, some of the BlueZ tools might not be bundled by default by your
Linux distribution. If you need to build BlueZ from scratch to update to a
recent version or to obtain all of its tools you can follow the steps below:

.. code-block:: console

   git clone git://git.kernel.org/pub/scm/bluetooth/bluez.git
   cd bluez
   ./bootstrap-configure --disable-android --disable-midi
   make

You can then find :file:`btattach`, :file:`btmgt` and :file:`btproxy` in the
:file:`tools/` folder and :file:`btmon` in the :file:`monitor/` folder.

You'll need to enable BlueZ's experimental features so you can access its
most recent BLE functionality. Do this by editing the file
:file:`/lib/systemd/system/bluetooth.service`
and making sure to include the :literal:`-E` option in the daemon's execution
start line:

.. code-block:: console

   ExecStart=/usr/libexec/bluetooth/bluetoothd -E

Finally, reload and restart the daemon:

.. code-block:: console

   sudo systemctl daemon-reload
   sudo systemctl restart bluetooth

.. _bluetooth_qemu_posix:

Running on QEMU and Native POSIX
********************************

It's possible to run Bluetooth applications using either the :ref:`QEMU
emulator<application_run_qemu>` or :ref:`Native POSIX <native_posix>`.
In either case, a Bluetooth controller needs to be exported from
the host OS (Linux) to the emulator. For this purpose you will need some tools
described in the :ref:`bluetooth_bluez` section.

Using the Host System Bluetooth Controller
==========================================

The host OS's Bluetooth controller is connected in the following manner:

* To the second QEMU serial line using a UNIX socket. This socket gets used
  with the help of the QEMU option :literal:`-serial unix:/tmp/bt-server-bredr`.
  This option gets passed to QEMU through :makevar:`QEMU_EXTRA_FLAGS`
  automatically whenever an application has enabled Bluetooth support.
* To a serial port in Native POSIX through the use of a command-line option
  passed to the Native POSIX executable: ``--bt-dev=hci0``

On the host side, BlueZ allows you to export its Bluetooth controller
through a so-called user channel for QEMU and Native POSIX to use.

.. note::
   You only need to run ``btproxy`` when using QEMU. Native POSIX handles
   the UNIX socket proxying automatically

If you are using QEMU, in order to make the Controller available you will need
one additional step using ``btproxy``:

#. Make sure that the Bluetooth controller is down

#. Use the btproxy tool to open the listening UNIX socket, type:

   .. code-block:: console

      sudo tools/btproxy -u -i 0
      Listening on /tmp/bt-server-bredr

   You might need to replace :literal:`-i 0` with the index of the Controller
   you wish to proxy.

   If you see ``Received unknown host packet type 0x00`` when running QEMU, then
   add :literal:`-z` to the ``btproxy`` command line to ignore any null bytes
   transmitted at startup.

Once the hardware is connected and ready to use, you can then proceed to
building and running a sample:

* Choose one of the Bluetooth sample applications located in
  :literal:`samples/bluetooth`.

* To run a Bluetooth application in QEMU, type:

  .. zephyr-app-commands::
     :zephyr-app: samples/bluetooth/<sample>
     :host-os: unix
     :board: qemu_x86
     :goals: run
     :compact:

  Running QEMU now results in a connection with the second serial line to
  the :literal:`bt-server-bredr` UNIX socket, letting the application
  access the Bluetooth controller.

* To run a Bluetooth application in Native POSIX, first build it:

  .. zephyr-app-commands::
     :zephyr-app: samples/bluetooth/<sample>
     :host-os: unix
     :board: native_posix
     :goals: build
     :compact:

  And then run it with::

     $ sudo ./build/zephyr/zephyr.exe --bt-dev=hci0

Using a Zephyr-based BLE Controller
===================================

Depending on which hardware you have available, you can choose between two
transports when building a single-mode, Zephyr-based BLE Controller:

* UART: Use the :ref:`hci_uart <bluetooth-hci-uart-sample>` sample and follow
  the instructions in :ref:`bluetooth-hci-uart-qemu-posix`.
* USB: Use the :ref:`hci_usb <bluetooth-hci-usb-sample>` sample and then
  treat it as a Host System Bluetooth Controller (see previous section)

HCI Tracing
===========

When running the Host on a computer connected to an external Controller, it
is very useful to be able to see the full log of exchanges between the two,
in the format of a :ref:`bluetooth-hci` log.
In order to see those logs, you can use the built-in ``btmon`` tool from BlueZ:

.. code-block:: console

   $ btmon

.. _bluetooth_ctlr_bluez:

Using Zephyr-based Controllers with BlueZ
*****************************************

If you want to test a Zephyr-powered BLE Controller using BlueZ's Bluetooth
Host, you will need a few tools described in the :ref:`bluetooth_bluez` section.
Once you have installed the tools you can then use them to interact with your
Zephyr-based controller:

   .. code-block:: console

      sudo tools/btmgmt --index 0
      [hci0]# auto-power
      [hci0]# find -l

You might need to replace :literal:`--index 0` with the index of the Controller
you wish to manage.
Additional information about :file:`btmgmt` can be found in its manual pages.


.. _nRF Connect for Android: https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp&hl=en
.. _nRF Connect for iOS: https://itunes.apple.com/us/app/nrf-connect/id1054362403
.. _LightBlue for Android: https://play.google.com/store/apps/details?id=com.punchthrough.lightblueexplorer&hl=en_US
.. _LightBlue for iOS: https://itunes.apple.com/us/app/lightblue-explorer/id557428110
.. _nRF Mesh for Android: https://play.google.com/store/apps/details?id=no.nordicsemi.android.nrfmeshprovisioner&hl=en
.. _nRF Mesh for iOS: https://itunes.apple.com/us/app/nrf-mesh/id1380726771
