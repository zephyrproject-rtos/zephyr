.. zephyr:code-sample:: delta-fw-update
   :name: Delta firmware update

   Apply a delta patch to update firmware using the bsdiff+heatshrink
   delta backend.

Overview
********

This sample demonstrates a delta firmware update using the generic
delta patching primitive (:file:`lib/delta`) with the
bsdiff+heatshrink backend (:c:data:`delta_backend_bsdiffhs`).

The sample provides the storage glue itself: it reads the source image
from ``slot0_partition``, reads the patch from a dedicated
``patch_partition`` on demand, and writes the reconstructed image into
the MCUboot upload slot via :c:func:`flash_img_buffered_write` — no
in-memory copy of the patch.

Requirements
************

- A board with at least four flash partitions:

  - ``boot_partition``: MCUboot bootloader
  - ``slot0_partition``: current running firmware (delta source)
  - ``slot1_partition``: MCUboot upload slot (delta target)
  - ``patch_partition``: storage for the delta patch

- MCUboot bootloader
- Heatshrink library (external west module)

Adding the Heatshrink Module
============================

Heatshrink is not yet part of the standard Zephyr manifest (its
integration as a Zephyr module is in progress). Until then, add it to
your workspace via a submanifest:

.. code-block:: yaml

   # submanifests/heatshrink.yml
   manifest:
     projects:
       - name: heatshrink
         url: https://github.com/kickmaker/heatshrink
         revision: c47356093c4ea079749214de2864186a578668a0
         path: modules/lib/heatshrink

Then run ``west update heatshrink``.

Building and Running
********************

This sample requires sysbuild to build MCUboot alongside the
application.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/dfu/delta
   :board: <board>
   :goals: build
   :west-args: --sysbuild
   :compact:

Generating a Patch
==================

To generate a delta patch between two firmware versions:

1. Build the source firmware (current version) and flash it::

      west build -b <board> samples/subsys/dfu/delta/ --sysbuild -p
      west flash

2. Store the signed source binary::

      cp build/delta/zephyr/zephyr.signed.bin \
         samples/subsys/dfu/delta/firmwares/source.bin

3. Build the target firmware with
   :kconfig:option:`CONFIG_DELTA_SAMPLE_TARGET_MODE` enabled::

      west build -b <board> samples/subsys/dfu/delta/ --sysbuild -p \
          -- -Ddelta_CONFIG_DELTA_SAMPLE_TARGET_MODE=y

   The target-mode build prints a confirmation message and exits
   without running the delta update logic, so it produces a different
   binary from the source.

4. Store the signed target binary::

      cp build/delta/zephyr/zephyr.signed.bin \
         samples/subsys/dfu/delta/firmwares/target.bin

5. Install the host tooling and generate the patch::

      pip install -r ./samples/subsys/dfu/delta/scripts/requirements.txt

      python3 samples/subsys/dfu/delta/scripts/generate_patch.py \
          samples/subsys/dfu/delta/firmwares/source.bin \
          samples/subsys/dfu/delta/firmwares/target.bin \
          samples/subsys/dfu/delta/firmwares/patch.bin \
          10 4 50000

   Positional arguments:

   - ``fw_source``: source firmware binary (current version).
   - ``fw_target``: target firmware binary (new version).
   - ``patch_file``: output patch file path.
   - ``window_sz2``: heatshrink window exponent (``10`` = 2^10 = 1024).
     Must **equal** :kconfig:option:`CONFIG_HEATSHRINK_STATIC_WINDOW_BITS`
     on-device (static allocation mode).
   - ``lookahead_sz2``: heatshrink lookahead exponent (``4`` = 2^4 = 16).
     Must **equal**
     :kconfig:option:`CONFIG_HEATSHRINK_STATIC_LOOKAHEAD_BITS`.
   - ``max_size_patch``: maximum allowed patch size in bytes (e.g.
     ``50000``). The script aborts if the generated patch exceeds
     this limit.

Flashing the Patch
==================

A ``flash_patch`` build target converts the patch binary to Intel HEX
at the devicetree-resolved ``patch_partition`` address and flashes it
via ``west flash``.

By default it expects the patch at
``samples/subsys/dfu/delta/firmwares/patch.bin``::

   west build -t flash_patch --domain delta

To use a different patch file::

   west build -t flash_patch --domain delta -- -DPATCH_FILE=path/to/my_patch.bin

Sample Output
=============

On a successful update:

.. code-block:: console

   [00:00:00.000,000] <inf> delta_sample: Delta sample
   [00:00:05.000,000] <inf> delta_apply: New firmware written, rebooting...

After reboot, MCUboot swaps the slots and boots the new firmware.

When built with :kconfig:option:`CONFIG_DELTA_SAMPLE_TARGET_MODE`
enabled:

.. code-block:: console

   [00:00:00.000,000] <inf> delta_sample: Delta target firmware

Configuration Options
*********************

- :kconfig:option:`CONFIG_DELTA`: enable the generic delta patching
  library.
- :kconfig:option:`CONFIG_DELTA_BACKEND_BSDIFFHS`: enable the
  bsdiff+heatshrink backend.
- :kconfig:option:`CONFIG_HEATSHRINK_STATIC_WINDOW_BITS`,
  :kconfig:option:`CONFIG_HEATSHRINK_STATIC_LOOKAHEAD_BITS`,
  :kconfig:option:`CONFIG_HEATSHRINK_STATIC_INPUT_BUFFER_SIZE`:
  compile-time sizing of the heatshrink decoder. A patch declaring
  different window/lookahead values is rejected with ``-ENOTSUP``.
- :kconfig:option:`CONFIG_DELTA_SAMPLE_TARGET_MODE`: build the sample
  as the target firmware for patch generation.
