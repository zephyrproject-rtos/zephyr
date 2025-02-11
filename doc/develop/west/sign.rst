.. _west-sign:

Signing Binaries
################

The ``west sign`` :ref:`extension <west-extensions>` command can be used to
sign a Zephyr application binary for consumption by a bootloader using an
external tool. In some configurations, ``west sign`` is also used to invoke
an external, post-processing tool that "stitches" the final components of
the image together. Run ``west sign -h`` for command line help.

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
  provided and used by MCUboot for development and testing
- You can change the ``hello_world`` application directory to any other
  application that can be loaded by MCUboot, such as the :zephyr:code-sample:`smp-svr` sample.

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

.. _west-extending-signing:

Extending signing externally
****************************

The signing script used when running ``west flash`` can be extended or replaced
to change features or introduce different signing mechanisms. By default with
MCUboot enabled, signing is setup by the :file:`cmake/mcuboot.cmake` file in
Zephyr which adds extra post build commands for generating the signed images.
The file used for signing can be replaced from a sysbuild scope (if being used)
or from a zephyr/zephyr module scope, the priority of which is:

* Sysbuild
* Zephyr property
* Default MCUboot script (if enabled)

From sysbuild, ``-D<target>_SIGNING_SCRIPT`` can be used to set a signing script
for a specific image or ``-DSIGNING_SCRIPT`` can be used to set a signing script
for all images, for example:

.. code-block:: console

   west build -b <board> <application> -DSIGNING_SCRIPT=<file>

The zephyr property method is achieved by adjusting the ``SIGNING_SCRIPT`` property
on the `zephyr_property_target`, ideally from by a module by using:

.. code-block:: cmake

   if(CONFIG_BOOTLOADER_MCUBOOT)
     set_target_properties(zephyr_property_target PROPERTIES SIGNING_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/custom_signing.cmake)
   endif()

This will include the custom signing CMake file instead of the default Zephyr
one when projects are built with MCUboot signing support enabled. The base
Zephyr MCUboot signing file can be used as a reference for creating a new
signing system or extending the default behaviour.

.. _MCUboot:
   https://mcuboot.com/

.. _imgtool:
   https://pypi.org/project/imgtool/


rimage
******

rimage configuration uses a different approach that does not rely on Kconfig or CMake
but on :ref:`west config<west-config>` instead, similar to
:ref:`west-building-cmake-config`.

Signing involves a number of "wrapper" scripts stacked on top of each other: ``west
flash`` invokes ``west build`` which invokes ``cmake`` and ``ninja`` which invokes
``west sign`` which invokes ``imgtool`` or `rimage`_. As long as the signing
parameters desired are the default ones and fairly static, these indirections are
not a problem. On the other hand, passing ``imgtool`` or ``rimage`` options through
all these layers can causes issues typical when the layers don't abstract
anything. First, this usually requires boilerplate code in each layer. Quoting
whitespace or other special characters through all the wrappers can be
difficult. Reproducing a lower ``west sign`` command to debug some build-time issue
can be very time-consuming: it requires at least enabling and searching verbose
build logs to find which exact options were used. Copying these options from the
build logs can be unreliable: it may produce different results because of subtle
environment differences. Last and worst: new signing feature and options are
impossible to use until more boilerplate code has been added in each layer.

To avoid these issues, ``rimage`` parameters can bet set in ``west config``
instead. Here's a ``workspace/.west/config`` example:

.. code-block:: ini

   [sign]
   # Not needed when invoked from CMake
   tool = rimage

   [rimage]
   # Quoting is optional and works like in Unix shells
   # Not needed when rimage can be found in the default PATH
   path = "/home/me/zworkspace/build-rimage/rimage"

   # Not needed when using the default development key
   extra-args = -i 4 -k 'keys/key argument with space.pem'

In order to support quoting, values are parsed by Python's ``shlex.split()`` like in
:ref:`west-building-cmake-args`.

The ``extra-args`` are passed directly to the ``rimage`` command. The example
above has the same effect as appending them on command line after ``--`` like this:
``west sign --tool rimage -- -i 4 -k 'keys/key argument with space.pem'``. In case
both are used, the command-line arguments go last.

.. _rimage:
   https://github.com/thesofproject/rimage
