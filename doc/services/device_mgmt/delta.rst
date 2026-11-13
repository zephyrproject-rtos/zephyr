.. _delta:

Delta firmware updates
######################

Overview
********

The delta patching framework provides a storage-agnostic primitive
for reconstructing a new byte stream from an existing one plus a
compact patch. It is used to reduce the OTA payload size of firmware
updates: the device receives only the binary difference between the
running image and the new image, then reconstructs the new image
locally.

The library
===========

:zephyr_file:`lib/delta/` contains the core, which knows nothing about
flash, file systems, MCUboot, or any specific diff/compression
algorithm. The caller supplies three offset-based I/O callbacks
(source, patch, target) plus an opaque work buffer, then selects a
backend via :c:struct:`delta_backend`. The core in
:c:func:`delta_apply_patch` validates arguments and dispatches.

The first backend shipped is :c:data:`delta_backend_bsdiffhs`, which
implements bsdiff for the binary diffing and heatshrink for the patch
stream compression. Adding another backend is a new subdirectory under
:zephyr_file:`lib/delta/backends/` and a new ``backend_<name>.h``
header alongside :zephyr_file:`include/zephyr/delta/backend_bsdiffhs.h`,
with no change to the core.

Applying a delta
****************

The application wires the three :c:struct:`delta_io` callbacks to its
own storage and calls :c:func:`delta_apply_patch`. An optional fourth
callback, ``finalize``, is invoked once after the whole target has
been written. For a
standard MCUboot dual-slot update — source read from a flash area,
target written to the upload slot via
:c:func:`flash_img_buffered_write`, flushed from ``finalize`` — see
the example in :zephyr_file:`samples/subsys/dfu/delta`.

Configuration
*************

* :kconfig:option:`CONFIG_DELTA` — enable the generic patching lib.

Each backend contributes its own Kconfig options; for the shipped
backend see `The bsdiffhs backend`_.

The bsdiffhs backend
********************

:c:data:`delta_backend_bsdiffhs` is the first backend shipped with
``lib/delta``. It pairs bsdiff binary diffing with heatshrink
compression of the patch streams. Everything in this section is
specific to this backend; the generic library above depends on none
of it.

Generating a patch
==================

Patches are produced on the host with the ``bsdiffhs`` Python
library. The helper script
:zephyr_file:`samples/subsys/dfu/delta/scripts/generate_patch.py`
wraps the library and emits a patch carrying the 18-byte extended
BSDIFFHS header consumed by the device:

.. code-block:: shell

   pip install bsdiffhs
   python3 samples/subsys/dfu/delta/scripts/generate_patch.py \
       source.bin target.bin patch.bin <window_sz2> <lookahead_sz2> <max_size>

When the device decoder uses static allocation (the default), the
``window_sz2`` and ``lookahead_sz2`` arguments must equal the device's
compile-time values (see `Backend configuration and memory`_).

External module dependency: heatshrink
======================================

The bsdiffhs backend depends on the heatshrink compression library,
provided as an external module checked out under
``modules/lib/heatshrink`` in the west workspace. Enabling
:kconfig:option:`CONFIG_DELTA_BACKEND_BSDIFFHS` selects
:kconfig:option:`CONFIG_HEATSHRINK`, which brings in the module code.
See :ref:`modules` for background on external modules, and the sample
documentation for how to add heatshrink to a workspace while its
integration in the Zephyr manifest is in progress.

Backend configuration and memory
================================

* :kconfig:option:`CONFIG_DELTA_BACKEND_BSDIFFHS` — ship the
  bsdiff+heatshrink backend (selects
  :kconfig:option:`CONFIG_HEATSHRINK`).
* :kconfig:option:`CONFIG_HEATSHRINK_STATIC_WINDOW_BITS` /
  ``_LOOKAHEAD_BITS`` / ``_INPUT_BUFFER_SIZE`` — compile-time sizing
  of the heatshrink decoder. A patch declaring different
  window/lookahead values is rejected.

The whole caller-supplied ``work_buf`` (at least 16 bytes) is used as
the patch read-ahead window; a larger buffer means fewer patch reads.
The backend additionally keeps two 256-byte chunk buffers on the
stack, plus the heatshrink decoder itself in static allocation mode
(about ``2^CONFIG_HEATSHRINK_STATIC_WINDOW_BITS`` +
``CONFIG_HEATSHRINK_STATIC_INPUT_BUFFER_SIZE`` bytes). With
:kconfig:option:`CONFIG_HEATSHRINK_DYNAMIC_ALLOC` the decoder is
allocated from the heap and sized from the patch header instead.

API reference
*************

Generic delta patching (lib/delta)
==================================

.. doxygengroup:: delta_api

bsdiff + heatshrink backend
===========================

.. doxygengroup:: delta_backend_bsdiffhs
