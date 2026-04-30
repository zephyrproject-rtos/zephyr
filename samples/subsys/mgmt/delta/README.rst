.. zephyr:code-sample:: dfota
   :name: Delta Firmware Over The Air Update

   Apply a delta patch to update firmware using the bsdiff algorithm.

Overview
********

This sample demonstrates the Delta Firmware Over The Air (DFOTA) update
capability. DFOTA allows updating device firmware by applying a delta patch
instead of downloading a complete firmware image, significantly reducing
the data transfer size.

The sample uses:

- **bsdiff algorithm**: A binary diff algorithm that creates compact patches
- **heatshrink compression**: A lightweight compression library suitable for
  embedded systems
- **MCUboot**: As the bootloader to manage firmware slots and boot process

Requirements
************

- A board with at least 3 flash partitions:

  - ``boot_partition``: MCUboot bootloader
  - ``slot0_partition``: Current running firmware
  - ``slot1_partition``: Target partition for new firmware
  - ``patch_partition``: Storage for the delta patch

- MCUboot bootloader
- Heatshrink library (external module)

Adding the Heatshrink Module
============================

The heatshrink compression library is required but not yet part of
the Zephyr manifest. Add it to your workspace manually:

.. code-block:: console

   west config manifest.project-filter -- +heatshrink
   west update heatshrink

.. note::

   The above commands will work once heatshrink is added to the Zephyr
   optional modules manifest. In the meantime, add the following to a
   file ``submanifests/heatshrink.yml`` in your west workspace root:

   .. code-block:: yaml

      manifest:
        projects:
          - name: heatshrink
            url: https://github.com/kickmaker/heatshrink
            revision: c47356093c4ea079749214de2864186a578668a0
            path: modules/lib/heatshrink

   Then run ``west update heatshrink``.

Building and Running
********************

This sample requires sysbuild to build MCUboot alongside the application.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mgmt/delta
   :board: <board>
   :goals: build
   :west-args: --sysbuild
   :compact:

Generating a Patch
==================

To generate a delta patch between two firmware versions:

1. Build the source firmware (current version) and flash it to your device::

      west build -b <board> samples/subsys/mgmt/delta/ --sysbuild -p

      west flash

2. Store your source binary::

      cp build/delta/zephyr/zephyr.signed.bin samples/subsys/mgmt/delta/firmwares/source.bin

3. Build the target firmware with ``CONFIG_DELTA_SAMPLE_TARGET_MODE`` enabled::

      west build -b <board> samples/subsys/mgmt/delta/ --sysbuild -p \
          -- -DCONFIG_DELTA_SAMPLE_TARGET_MODE=y

   This builds a firmware that skips the delta update logic and simply
   prints a confirmation message, allowing you to produce a different
   binary for patch generation.

4. Store your target binary::

      cp build/delta/zephyr/zephyr.signed.bin samples/subsys/mgmt/delta/firmwares/target.bin

5. Install the Python dependencies and generate the patch::

      pip install -r ./samples/subsys/mgmt/delta/scripts/requirements.txt

      python3 samples/subsys/mgmt/delta/scripts/generate_patch.py \
          samples/subsys/mgmt/delta/firmwares/source.bin \
          samples/subsys/mgmt/delta/firmwares/target.bin \
          samples/subsys/mgmt/delta/firmwares/patch.bin \
          10 4 50000

   The positional arguments are:

   - ``fw_source``: source firmware binary (current version).
   - ``fw_target``: target firmware binary (new version).
   - ``patch_file``: output patch file path.
   - ``window_sz2``: heatshrink window size exponent (``10`` = 2^10 = 1024 bytes).
     Must match :kconfig:option:`CONFIG_HEATSHRINK_STATIC_WINDOW_BITS` when
     using static allocation.
   - ``lookahead_sz2``: heatshrink lookahead size exponent (``4`` = 2^4 = 16 bytes).
     Must match :kconfig:option:`CONFIG_HEATSHRINK_STATIC_LOOKAHEAD_BITS` when
     using static allocation.
   - ``max_size_patch``: maximum allowed patch size in bytes (e.g. ``50000``).
     The script exits with an error if the generated patch exceeds this limit.


Flashing the Patch
==================

A ``flash_patch`` build target is provided to flash the patch binary to
the ``patch_partition``. The target automatically resolves the partition
address from the devicetree, converts the binary to Intel HEX format,
and flashes it using ``west flash``.

By default the target expects the patch at
``samples/subsys/mgmt/delta/firmwares/patch.bin``::

   west build -t flash_patch --domain delta

To use a different patch file::

   west build -t flash_patch --domain delta -- -DPATCH_FILE=path/to/my_patch.bin

Sample Output
=============

When the sample runs and detects a valid patch:

.. code-block:: console

   [00:00:00.000,000] <inf> delta_sample: Delta sample
   [00:00:00.000,000] <inf> delta_apply: Valid patch detected in patch_partition
   [00:00:00.000,000] <inf> delta_apply: Target size: 65536, window_sz2: 10, lookahead_sz2: 4
   [00:00:05.000,000] <inf> delta_apply: The new FW was successfully written, now rebooting...

After reboot, MCUboot will boot into the new firmware.

If no valid patch is present:

.. code-block:: console

   [00:00:00.000,000] <inf> delta_sample: Delta sample
   [00:00:00.000,000] <inf> delta_apply: Wrong magic in the patch
   [00:00:00.000,000] <inf> delta_apply: No valid patch in patch_partition
   [00:00:00.000,000] <err> delta_sample: Error while applying delta update: -140

When built with ``CONFIG_DELTA_SAMPLE_TARGET_MODE=y``:

.. code-block:: console

   [00:00:00.000,000] <inf> delta_sample: DFOTA target firmware

Configuration Options
*********************

- :kconfig:option:`CONFIG_DELTA_UPDATE`: Enable delta firmware update support
- :kconfig:option:`CONFIG_BSDIFF_HS_ALGO`: Use bsdiff algorithm and heatshrink
  compression for patching
- :kconfig:option:`CONFIG_HEATSHRINK`: Enable heatshrink compression
- :kconfig:option:`CONFIG_DELTA_INPUT_BUFFER_SIZE`: Size of the read-ahead buffer
  for patch decompression (default 64 bytes)
- :kconfig:option:`CONFIG_HEATSHRINK_DYNAMIC_ALLOC`: Use dynamic allocation for
  heatshrink decoder (recommended for variable window sizes)
- :kconfig:option:`CONFIG_DELTA_SAMPLE_TARGET_MODE`: Build the sample as the
  target firmware for delta patch generation
