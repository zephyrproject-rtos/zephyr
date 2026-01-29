.. zephyr:code-sample:: ec_host_cmd_usb
   :name: EC host commands over USB
   :relevant-api: ec_host_cmd_interface

   Example EC host command over USB implementation

Overview
********

This samples implements some basic EC host commands using the EC host commands
over USB subsystem. It can be used as a starting point for a simple, self
contained application used to test or develop host side code corresponding to a
specific host command.

Building and Running
********************

Build and flash as follows, changing ``frdm_mcxn947/mcxn947/cpu0`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mgmt/ec_host_cmd_usb
   :board: frdm_mcxn947/mcxn947/cpu0
   :goals: build flash
   :compact:

After starting, the device should connect via USB with the vendor specific interface:

.. code-block:: console

    $ lsusb -vv -d 2fe3:0001
    ...
        Interface Descriptor:
          bLength                 9
          bDescriptorType         4
          bInterfaceNumber        0
          bAlternateSetting       0
          bNumEndpoints           3
          bInterfaceClass       255 Vendor Specific Class
          bInterfaceSubClass     90 [unknown]
          bInterfaceProtocol      0
          iInterface              0

Running a Linux image against it
********************************

The sample can be used to run Linux drivers using the ``cros_ec_dev`` MFD device
against it. To test that, first you need to pach the ChromeOS EC USB driver
into a Linux kernel:

.. code-block:: console

    curl https://lore.kernel.org/chrome-platform/20250912065104.260344-1-dawidn@google.com/t.mbox.gz | gunzip | git am

Then build a kernel with the CROS_EC options enabled:

.. code-block:: console

    make defconfig
    ./scripts/config -e CONFIG_CHROME_PLATFORMS -e CONFIG_CROS_EC -e CONFIG_CROS_EC_USB -e CONFIG_KEYBOARD_CROS_EC
    make olddefconfig
    make -j10

Then the image can be ran on qemu directly (note: use the system qemu, not the
one in Zephyr SDK):

.. code-block:: console

    sudo /usr/bin/qemu-system-x86_64 \
            --m 1024 \
            --machine q35 \
            --cpu host \
            --smp 2 \
            --display gtk \
            --accel kvm \
            \
            --device virtio-serial-pci \
            --chardev stdio,id=char0 \
            --device virtconsole,chardev=char0 \
            \
            --device virtio-net-pci,netdev=net0 \
            --netdev user,id=net0 \
            \
            --device qemu-xhci \
            --device usb-host,vendorid=0x18d1,productid=0x0204 \
            --device usb-host,vendorid=0x18d1,productid=0x5214 \
            --device usb-host,vendorid=0x2fe3,productid=0x0001 \
            \
            --drive file=debian-13-nocloud-amd64-20251117-2299.qcow2,if=none,id=drive-vda \
            --device virtio-blk-pci,drive=drive-vda,serial=qemu-vda \
            \
            --kernel $HOME/linux/arch/x86/boot/bzImage \
            --append "root=/dev/vda1 console=hvc0"

This is using a pre-built nocloud Debian image from
https://cloud.debian.org/images/cloud/trixie/

In the gues check that the USB device has been mapped to the guest OS, and bind
the cros-ec-usb to it.

.. code-block:: console

    root@localhost:~# lsusb
    Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
    Bus 001 Device 002: ID 2fe3:0001 NordicSemiconductor USBD sample
    Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
    root@localhost:~# echo 2fe3 0001 > /sys/bus/usb/drivers/cros-ec-usb/new_id
    ...
    [   35.252406] cros-ec-usb 1-3:1.0: Chrome EC device registered
    ...

You should now be able to test the EC HC interface with

.. code-block:: console

    root@localhost:~# cat /sys/kernel/debug/cros_ec/uptime
    904270

Testing sub-drivers
*******************

Most cros-ec-dev subdrivers are instantiated from
``drivers/mfd/cros_ec_dev.c``, though few of them can only be instantiated via
devicetree. The simplest way to test them is to hack their data in the
``cros_ec_dev`` file as well, for example the ``cros-ec-keyb`` driver that is
normally using a devicetree node:

.. code-block:: devicetree

    #include <dt-bindings/input/cros-ec-keyboard.h>

    &cros_ec {
            keyboard_controller: keyboard-controller {
                    compatible = "google,cros-ec-keyb";
                    keypad,num-rows = <8>;
                    keypad,num-columns = <8>;

                    linux,keymap = <
                            MATRIX_KEY(1, 0, KEY_1),
                            MATRIX_KEY(2, 0, KEY_2),
                    >;
            };
    };

can be instantiated manually with

.. code-block:: c

    #include <linux/input-event-codes.h>
    #include <linux/input/matrix_keypad.h>

    static const uint32_t keyb_keymap[] = {
            KEY(1, 0, KEY_1),
            KEY(2, 0, KEY_2),
    };

    static const struct property_entry keyb_properties[] = {
        PROPERTY_ENTRY_U32("keypad,num-rows", 8),
        PROPERTY_ENTRY_U32("keypad,num-columns", 8),
        PROPERTY_ENTRY_U32_ARRAY("linux,keymap", keyb_keymap),
        {},
    };

    static const struct software_node keyb_swnode = {
           .properties = keyb_properties,
    };

    static const struct mfd_cell keyb_cell = {
           .name = "cros-ec-keyb",
           .swnode = &keyb_swnode,
    };

    /* in ec_device_probe */
    mfd_add_hotplug_devices(ec->dev->parent, &keyb_cell, 1);
