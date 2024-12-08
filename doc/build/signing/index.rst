.. _build-signing:

Signing Binaries
################

Binaries can be optionally signed as part of a build automatically using CMake code, there is
also the ability to use ``west sign`` to sign binaries too, this page describes the former, the
latter is documented on :ref:`west-sign`.

MCUboot / imgtool
*****************

The Zephyr build system has special support for signing binaries for use with the `MCUboot`_
bootloader using the `imgtool`_ program provided by its developers. You can both build and sign
this type of application binary in one step by setting some Kconfig options. If you do,
``west flash`` will use the signed binaries.

Here is an example workflow, which builds and flashes MCUboot, as well as the
:zephyr:code-sample:`hello_world` application for chain-loading by MCUboot. Run these commands
from the :file:`zephyrproject` workspace you created in the :ref:`getting_started`.

.. code-block:: console

   west build -b YOUR_BOARD zephyr/samples/hello_world --sysbuild -d build-hello-signed -- \
        -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

   west flash -d build-hello-signed

Notes on the above commands:

- ``YOUR_BOARD`` should be changed to match your board
- The singing key value is the insecure default provided and used by MCUboot for development
  and testing
- You can change the ``hello_world`` application directory to any other application that can be
  loaded by MCUboot, such as the :zephyr:code-sample:`smp-svr` sample.

For more information on these and other related configuration options, see:

- ``SB_CONFIG_BOOTLOADER_MCUBOOT``: build the application for loading by MCUboot
- ``SB_CONFIG_BOOT_SIGNATURE_KEY_FILE``: the key file to use when singing images. If you have
  your own key, change this appropriately
- :kconfig:option:`CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS`: optional additional command line arguments
  for ``imgtool``
- :kconfig:option:`CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE`: also generate a confirmed image,
  which may be more useful for flashing in production environments than the OTA-able default image
- On Windows, if you get "Access denied" issues, the recommended fix is to run
  ``pip3 install imgtool``, then retry with a pristine build directory.

If your ``west flash`` :ref:`runner <west-runner>` uses an image format supported by imgtool, you
should see something like this on your device's serial console when you run
``west flash -d build-hello-signed``:

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

Whether ``west flash`` supports this feature depends on your runner. The ``nrfjprog`` and
``pyocd`` runners work with the above flow. If your runner does not support this flow and you
would like it to, please send a patch or file an issue for adding support.

.. _west-extending-signing:

Extending signing externally
****************************

The signing script used when running ``west flash`` can be extended or replaced to change features
or introduce different signing mechanisms. By default with MCUboot enabled, signing is setup by
the :file:`cmake/mcuboot.cmake` file in Zephyr which adds extra post build commands for generating
the signed images. The file used for signing can be replaced from a sysbuild scope (if being used)
or from a zephyr/zephyr module scope, the priority of which is:

* Sysbuild
* Zephyr property
* Default MCUboot script (if enabled)

From sysbuild, ``-D<target>_SIGNING_SCRIPT`` can be used to set a signing script for a specific
image or ``-DSIGNING_SCRIPT`` can be used to set a signing script for all images, for example:

.. code-block:: console

   west build -b <board> <application> -DSIGNING_SCRIPT=<file>

The zephyr property method is achieved by adjusting the ``SIGNING_SCRIPT`` property on the
``zephyr_property_target``, ideally from by a module by using:

.. code-block:: cmake

   if(CONFIG_BOOTLOADER_MCUBOOT)
     set_target_properties(zephyr_property_target PROPERTIES SIGNING_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/custom_signing.cmake)
   endif()

This will include the custom signing CMake file instead of the default Zephyr one when projects
are built with MCUboot signing support enabled. The base Zephyr MCUboot signing file can be
used as a reference for creating a new signing system or extending the default behaviour.

.. _MCUboot:
   https://mcuboot.com/

.. _imgtool:
   https://pypi.org/project/imgtool/
