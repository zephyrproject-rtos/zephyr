.. zephyr:code-sample:: bootargs
   :name: Bootargs

   Print received bootargs to the console.

Overview
********

This sample demonstrates use of bootargs passed to Zephyr by printing each main argument to the console.
Zephyr support both dynamic bootargs, received from supported bootloader, and static bootargs embedded in the binary.

Requirements
************

Static bootargs don't have special requirements.
Dynamic bootargs work on platforms where Zephyr is booted by multiboot or efi.

Building and Running
********************

Static bootargs
===============

Static bootargs can be configured using ``CONFIG_BOOTARGS_STRING``.

.. zephyr-app-commands::
   :zephyr-app: samples/kernel/bootargs
   :board: qemu_x86
   :conf: prj_static_bootargs.conf
   :goals: build run
   :compact:

Output:

.. code-block:: console

    *** Booting Zephyr OS build v3.7.0-514-gd4490bc739d1 ***
    argv[0] = appname
    argv[1] = This
    argv[2] = is
    argv[3] = a list of
    argv[4] = arguments

Multiboot
=========

.. zephyr-app-commands::
   :zephyr-app: samples/kernel/bootargs
   :board: qemu_x86
   :conf: prj_multiboot.conf
   :goals: build run
   :compact:

Output:

.. code-block:: console

    *** Booting Zephyr OS build v3.7.0-rc2-421-g3cf718e6dabc ***
    argv[0] = /home/user/zephyr/samples/kernel/bootargs/build/zephyr/zephyr.elf

To pass your own arguments you can manually invoke qemu with ``-append "your args"``, for example:

.. code-block:: console

    qemu-system-x86_64 -kernel ./build/zephyr/zephyr.elf -nographic -append "This is 'a list of' arguments"

Which will result in the following output:

.. code-block:: console

    *** Booting Zephyr OS build v3.7.0-rc2-421-g3cf718e6dabc ***
    argv[0] = ./build/zephyr/zephyr.elf
    argv[1] = This
    argv[2] = is
    argv[3] = a list of
    argv[4] = arguments

Efi
=========

.. zephyr-app-commands::
   :zephyr-app: samples/kernel/bootargs
   :board: qemu_x86_64
   :conf: prj_efi.conf
   :goals: build run
   :compact:

Output:

.. code-block:: console

    *** Zephyr EFI Loader ***
    RSDP found at 0xbf7e014
    Zeroing 501792 bytes of memory at 0x163000
    Copying 16384 data bytes to 0x1000 from image offset
    Copying 405504 data bytes to 0x100000 from image offset 16384
    Copying 30688 data bytes to 0x1dd820 from image offset 421888
    Jumping to Entry Point: 0x1137 (48 31 c0 48 31 d2 48)

    *** Booting Zephyr OS build v3.7.0-rc2-421-g3cf718e6dabc ***
    argv[0] = run.efi

To pass your own arguments you can press ESC and write your arguments after name of the Zephyr efi binary, for example:

.. code-block:: console

    Press ESC in 5 seconds to skip startup.nsh or any other key to continue.
    Shell> run.efi This is 'a list of' arguments
    *** Zephyr EFI Loader ***
    RSDP found at 0xbf7e014
    Zeroing 501792 bytes of memory at 0x163000
    Copying 16384 data bytes to 0x1000 from image offset
    Copying 405504 data bytes to 0x100000 from image offset 16384
    Copying 30688 data bytes to 0x1dd820 from image offset 421888
    Jumping to Entry Point: 0x1137 (48 31 c0 48 31 d2 48)

    *** Booting Zephyr OS build v3.7.0-rc2-421-g3cf718e6dabc ***
    argv[0] = run.efi
    argv[1] = This
    argv[2] = is
    argv[3] = a list of
    argv[4] = arguments
