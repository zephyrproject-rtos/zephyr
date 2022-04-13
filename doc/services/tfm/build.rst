.. _tfm_build_system:

TF-M Build System
#################

When building a valid ``_ns`` board target, TF-M will be built in the
background, and linked with the Zephyr non-secure application. No knowledge
of TF-M's build system is required in most cases, and the following will
build a TF-M and Zephyr image pair, and run it in qemu with no additional
steps required:

   .. code-block:: bash

     $ west build -p auto -t mps2_an521_ns samples/tfm_integration/psa_crypto/ -t run

The outputs and certain key steps in this build process are described here,
however, since you will need to understand and interact with the outputs, and
deal with signing the secure and non-secure images before deploying them.

Images Created by the TF-M Build
********************************

The TF-M build system creates the following executable files:

* tfm_s - TF-M secure firmware
* tfm_ns - TF-M non-secure app (only used by regression tests).
* bl2 - TF-M MCUboot, if enabled

For each of these, it creates .bin, .hex, .elf, and .axf files.

The TF-M build system also creates signed variants of tfm_s and tfm_ns, and a
file which combines them:

* tfm_s_signed
* tfm_ns_signed
* tfm_s_ns_signed

For each of these, only .bin files are created.

The TF-M non-secure app is discarded in favor of Zephyr non-secure app except
when running the TF-M regression test suite.

The Zephyr build system usually signs both tfm_s and the Zephyr non-secure app itself.
See below for details.

The 'tfm' target contains properties for all these paths.
For example, the following will resolve to ``<path>/tfm_s.hex``:

   .. code-block::

      $<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE>

See the top level CMakeLists.txt file in the tfm module for an overview of all
the properties.

Signing Images
**************

When :kconfig:option:`CONFIG_TFM_BL2` is set to ``y``, TF-M uses a secure bootloader
(BL2) and firmware images must be signed with a private key. The firmware image
is validated by the bootloader during updates using the corresponding public
key, which is stored inside the secure bootloader firmware image.

By default, ``<tfm-dir>/bl2/ext/mcuboot/root-rsa-3072.pem`` is used to sign secure
images, and ``<tfm-dir>/bl2/ext/mcuboot/root-rsa-3072_1.pem`` is used to sign
non-secure images. These default .pem keys can (and **should**) be overridden
using the :kconfig:option:`CONFIG_TFM_KEY_FILE_S` and
:kconfig:option:`CONFIG_TFM_KEY_FILE_NS` config flags.

To satisfy `PSA Certified Level 1`_ requirements, **You MUST replace
the default .pem file with a new key pair!**

To generate a new public/private key pair, run the following commands:

   .. code-block:: bash

     $ imgtool keygen -k root-rsa-3072_s.pem -t rsa-3072
     $ imgtool keygen -k root-rsa-3072_ns.pem -t rsa-3072

You can then place the new .pem files in an alternate location, such as your
Zephyr application folder, and reference them in the ``prj.conf`` file via the
:kconfig:option:`CONFIG_TFM_KEY_FILE_S` and :kconfig:option:`CONFIG_TFM_KEY_FILE_NS` config
flags.

   .. warning::

     Be sure to keep your private key file in a safe, reliable location! If you
     lose this key file, you will be unable to sign any future firmware images,
     and it will no longer be possible to update your devices in the field!

After the built-in signing script has run, it creates a ``tfm_merged.hex``
file that contains all three binaries: bl2, tfm_s, and the zephyr app. This
hex file can then be flashed to your development board or run in QEMU.

.. _PSA Certified Level 1:
  https://www.psacertified.org/security-certification/psa-certified-level-1/

Custom CMake arguments
======================

When building a Zephyr application with TF-M it might be necessary to control
the CMake arguments passed to the TF-M build.

Zephyr TF-M build offers several Kconfig options for controlling the build, but
doesn't cover every CMake argument supported by the TF-M build system.

The ``TFM_CMAKE_OPTIONS`` property on the ``zephyr_property_target`` can be used
to pass custom CMake arguments to the TF-M build system.

To pass the CMake argument ``-DFOO=bar`` to the TF-M build system, place the
following CMake snippet in your CMakeLists.txt file.

   .. code-block:: cmake

     set_property(TARGET zephyr_property_target
                  APPEND PROPERTY TFM_CMAKE_OPTIONS
                  -DFOO=bar
     )

.. note::
   The ``TFM_CMAKE_OPTIONS`` is a list so it is possible to append multiple
   options. Also CMake generator expressions are supported, such as
   ``$<1:-DFOO=bar>``

Footprint and Memory Usage
**************************

The build system offers targets to view and analyse RAM and ROM usage in generated images.
The tools run on the final images and give information about size of symbols and code being used in both RAM and ROM.
For more information on these tools look here: :ref:`footprint_tools`

Use the ``tfm_ram_report`` to get the RAM report for TF-M secure firmware (tfm_s).

.. zephyr-app-commands::
    :tool: all
    :app: samples/hello_world
    :board: mps2_an521_ns
    :goals: tfm_ram_report

Use the ``tfm_rom_report`` to get the ROM report for TF-M secure firmware (tfm_s).

.. zephyr-app-commands::
    :tool: all
    :app: samples/hello_world
    :board: mps2_an521_ns
    :goals: tfm_rom_report

Use the ``bl2_ram_report`` to get the RAM report for TF-M MCUboot, if enabled.

.. zephyr-app-commands::
    :tool: all
    :app: samples/hello_world
    :board: mps2_an521_ns
    :goals: bl2_ram_report

Use the ``bl2_rom_report`` to get the ROM report for TF-M MCUboot, if enabled.

.. zephyr-app-commands::
    :tool: all
    :app: samples/hello_world
    :board: mps2_an521_ns
    :goals: bl2_rom_report
