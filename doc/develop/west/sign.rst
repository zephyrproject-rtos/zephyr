.. _west-sign:

Signing Binaries
################

The ``west sign`` :ref:`extension <west-extensions>` command can be used to
sign a Zephyr application binary for consumption by a bootloader using an
external tool. Run ``west sign -h`` for command line help.

MCUboot / imgtool
*****************

The Zephyr build system has special support for signing binaries for use with
the `MCUboot`_ bootloader using the `imgtool`_ program provided by its
developers. You can both build and sign this type of application binary in one
step by setting some Kconfig options. If you do, ``west flash`` will use the
signed binaries.

If you use this feature, you don't need to run ``west sign`` yourself; the
build system will do it for you.

Here is an example workflow, which builds and flashes MCUboot, as well as the
:ref:`hello_world` application for chain-loading by MCUboot. Run these commands
from the :file:`zephyrproject` workspace you created in the
:ref:`getting_started`.

.. code-block:: console

   west build -b YOUR_BOARD bootloader/mcuboot/boot/zephyr -d build-mcuboot
   west build -b YOUR_BOARD zephyr/samples/hello_world -d build-hello-signed -- \
        -DCONFIG_BOOTLOADER_MCUBOOT=y \
        -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-rsa-2048.pem\"

   west flash -d build-mcuboot
   west flash -d build-hello-signed

Notes on the above commands:

- ``YOUR_BOARD`` should be changed to match your board
- The ``CONFIG_MCUBOOT_SIGNATURE_KEY_FILE`` value is the insecure default
  provided and used by by MCUboot for development and testing
- You can change the ``hello_world`` application directory to any other
  application that can be loaded by MCUboot, such as the :ref:`smp_svr_sample`

For more information on these and other related configuration options, see:

- :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`: build the application for loading by
  MCUboot
- :kconfig:option:`CONFIG_MCUBOOT_SIGNATURE_KEY_FILE`: the key file to use with ``west
  sign``. If you have your own key, change this appropriately
- :kconfig:option:`CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS`: optional additional command line
  arguments for ``imgtool``
- :kconfig:option:`CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE`: also generate a confirmed
  image, which may be more useful for flashing in production environments than
  the OTA-able default image
- On Windows, if you get "Access denied" issues, the recommended fix is
  to run ``pip3 install imgtool``, then retry with a pristine build directory.

If your ``west flash`` :ref:`runner <west-runner>` uses an image format
supported by imgtool, you should see something like this on your device's
serial console when you run ``west flash -d build-mcuboot``:

.. code-block:: none

   *** Booting Zephyr OS build zephyr-v2.3.0-2310-gcebac69c8ae1  ***
   [00:00:00.004,669] <inf> mcuboot: Starting bootloader
   [00:00:00.011,169] <inf> mcuboot: Primary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
   [00:00:00.021,636] <inf> mcuboot: Boot source: none
   [00:00:00.027,313] <wrn> mcuboot: Failed reading image headers; Image=0
   [00:00:00.035,064] <err> mcuboot: Unable to find bootable image

Then, you should see something like this when you run ``west flash -d
build-hello-signed``:

.. code-block:: none

   *** Booting Zephyr OS build zephyr-v2.3.0-2310-gcebac69c8ae1  ***
   [00:00:00.004,669] <inf> mcuboot: Starting bootloader
   [00:00:00.011,169] <inf> mcuboot: Primary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
   [00:00:00.021,636] <inf> mcuboot: Boot source: none
   [00:00:00.027,374] <inf> mcuboot: Swap type: none
   [00:00:00.115,142] <inf> mcuboot: Bootloader chainload address offset: 0xc000
   [00:00:00.123,168] <inf> mcuboot: Jumping to the first image slot
   *** Booting Zephyr OS build zephyr-v2.3.0-2310-gcebac69c8ae1  ***
   Hello World! nrf52840dk_nrf52840

Whether ``west flash`` supports this feature depends on your runner. The
``nrfjprog`` and ``pyocd`` runners work with the above flow. If your runner
does not support this flow and you would like it to, please send a patch or
file an issue for adding support.

.. _MCUboot:
   https://mcuboot.com/

.. _imgtool:
   https://pypi.org/project/imgtool/
