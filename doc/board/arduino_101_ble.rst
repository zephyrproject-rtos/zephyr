.. _arduino_101_ble:

Bluetooth firmware for the Arduino 101
######################################

The Arduino 101 board comes with a Nordic Semiconductor nRF51 Bluetooth
LE controller. The Arduino 101 factory-installed firmware on this
controller is not supported by Zephyr, so a new one needs to be flashed
onto it. The best option currently is to use MyNewt
(http://mynewt.apache.org) as a basis for this firmware, which makes it
possible to use the controller with the native Bluetooth stack of
Zephyr.

Building the firmware yourself
******************************

#. Set up the newt tool. For Fedora the dependencies are:

   .. code-block:: console

      $ sudo dnf install dfu-util libcurl-devel golang arm-none-eabi-gdb \
                arm-none-eabi-newlib arm-none-eabi-gcc-cs \
                arm-none-eabi-binutils-cs arm-none-eabi-gcc-cs-c++

   General instructions, as well as Ubuntu/Debian dependencies can be
   found here: http://mynewt.apache.org/newt/install/newt_linux/

#. Once the newt tool is installed create your project:

   .. code-block:: console

      $ newt new arduino-mynewt
      Downloading project skeleton from apache/incubator-mynewt-blinky...
      Installing skeleton in arduino-mynewt...
      Project arduino-mynewt successfully created.
      $ cd arduino-mynewt/

#. Currently the necessary code is only available in the 'develop'
   branch of MyNewt, aliased by version number '0.0.0'. To use this,
   edit :file:`project.yml` and change "vers: 0-latest" to
   "vers: 0-dev". After that, initialize the project with:

   .. code-block:: console

      $ newt install -v
      apache-mynewt-core
      Downloading repository description for apache-mynewt-core... success!
      Downloading repository incubator-mynewt-core (branch: master; commit: develop) at https://github.com/apache/incubator-mynewt-core.git
      Cloning into '/tmp/newt-repo604330290'...
      remote: Counting objects: 21889, done.
      remote: Compressing objects: 100% (97/97), done.
      remote: Total 21889 (delta 49), reused 0 (delta 0), pack-reused 21784
      Receiving objects: 100% (21889/21889), 7.63 MiB | 1.13 MiB/s, done.
      Resolving deltas: 100% (12945/12945), done.
      Checking connectivity... done.
      apache-mynewt-core successfully installed version 0.0.0-none

#. Create a target for the bootloader:

   .. code-block:: console

      $ newt target create boot
      $ newt target set boot app=@apache-mynewt-core/apps/boot
      $ newt target set boot bsp=@apache-mynewt-core/hw/bsp/nrf51-arduino_101
      $ newt target set boot build_profile=optimized

#. Create a target for the HCI over UART application:

   .. code-block:: console

      $ newt target create blehci
      $ newt target set blehci app=@apache-mynewt-core/apps/blehci
      $ newt target set blehci bsp=@apache-mynewt-core/hw/bsp/nrf51-arduino_101
      $ newt target set blehci build_profile=optimized

#. Verify that the targets were properly created:

   .. code-block:: console

      $ newt target show
      targets/blehci
          app=@apache-mynewt-core/apps/blehci
          bsp=@apache-mynewt-core/hw/bsp/nrf51-arduino_101
          build_profile=optimized
      targets/boot
          app=@apache-mynewt-core/apps/boot
          bsp=@apache-mynewt-core/hw/bsp/nrf51-arduino_101
          build_profile=optimized

#. Build the bootloader

   .. code-block:: console

      $ newt build boot
      Compiling...
      ...
      Linking boot.elf
      App successfully built: <path>/arduino-mynewt/bin/boot/apps/boot/boot.elf

#. Build the HCI over UART application:

   .. code-block:: console

      $ newt build blehci
      Compiling...
      ...
      Linking blehci.elf
      App successfully built: <path>/arduino-mynewt/bin/blehci/apps/blehci/blehci.elf

   .. code-block:: console

      $ newt create-image blehci 0.0.0
      App image succesfully generated: <path>/arduino-mynewt/bin/blehci/apps/blehci/blehci.img
      Build manifest: <path>/arduino-mynewt/bin/blehci/apps/blehci/manifest.json

#. Combine the bootloader and application into a single firmware image
   (:file:`ble_core.img`)

   .. code-block:: console

      $ cat bin/boot/apps/boot/boot.elf.bin /dev/zero | dd of=ble_core.img bs=1k count=256
      $ dd if=bin/blehci/apps/blehci/blehci.img of=ble_core.img bs=1 seek=32768

#. Reset Arduino 101 with USB plugged and wait a few seconds (you might
   need several repeated attempts):

   .. code-block:: console

      $ dfu-util -a ble_core -D ble_core.img
      ...
      Opening DFU capable USB device...
      ID 8087:0aba
      Run-time device DFU version 0011
      Claiming USB DFU Interface...
      Setting Alternate Setting #8 ...
      Determining device status: state = dfuIDLE, status = 0
      dfuIDLE, continuing
      DFU mode device DFU version 0011
      Device returned transfer size 2048
      Copying data from PC to DFU device
      Download	[=========================] 100%        69008 bytes
      Download done.
      state(2) = dfuIDLE, status(0) = No error condition is present
      Done!

After successfully completing these steps your Arduino 101 should now
have a HCI compatible BLE firmware. The Zephyr tree contains several
sample config files for this firmware (named after the MyNewt BLE stack,
Nimble), e.g. :file:`samples/bluetooth/peripheral_hr/prj_nimble.conf`

Getting HCI traces
******************

By default you will only see normal log messages on the console, without
any way of accessing the HCI traffic between Zephyr and the nRF51
controller. There is however a special Bluetooth logging mode that
converts the console to use a binary protocol that interleaves both
normal log messages as well as the HCI traffic. To enable this protocol
the following Kconfig options need to be set:

.. code-block:: none

   CONFIG_BLUETOOTH_DEBUG_MONITOR=y
   CONFIG_UART_CONSOLE=n
   CONFIG_UART_QMSI_1_BAUDRATE=1000000

The first item replaces the BLUETOOTH_DEBUG_STDOUT option, the second
one disables the default printk/printf hooks, and the third one matches
the console baudrate with what's used to communicate with the nRF51, in
order not to create a bottle neck.

To decode the binary protocol that will now be sent to the console UART
you need to use the btmon tool from `BlueZ 5.40
<http://www.bluez.org/release-of-bluez-5-40/>`_ or later:

.. code-block:: console

   $ btmon --tty <console TTY> --tty-speed 1000000
