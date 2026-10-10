.. _external_module_heatshrink:

heatshrink
##########

Introduction
************

`heatshrink`_ is a data compression and decompression library for embedded and real-time systems,
written by Scott Vokes at Atomic Object. It is based on LZSS and is designed for very low memory
usage (as low as 50 bytes of RAM), incremental and bounded CPU use, and either static or dynamic
memory allocation.

The `heatshrink Zephyr module`_ is a fork of the original project maintained by Kickmaker. It
adds the Zephyr integration (Kconfig options, CMake build rules and samples) on top of the
upstream library; the compression code itself comes from the original heatshrink project.

The heatshrink library is licensed under the ISC license. The Zephyr integration files are
licensed under the Apache-2.0 license.

Usage with Zephyr
*****************

To pull in heatshrink as a Zephyr module, either add it as a West project in the :file:`west.yml`
file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/heatshrink.yaml``) file
with the following content and run :command:`west update`:

.. code-block:: yaml

   manifest:
     projects:
       - name: heatshrink
         url: https://github.com/kickmaker/heatshrink.git
         revision: zephyr
         path: modules/lib/heatshrink # adjust the path as needed

Enable the library with ``CONFIG_HEATSHRINK``. By default, the encoder and decoder
use statically allocated state whose window and lookahead sizes are set at build time through
``CONFIG_HEATSHRINK_STATIC_WINDOW_BITS`` and ``CONFIG_HEATSHRINK_STATIC_LOOKAHEAD_BITS``. Data must
be decoded with the same parameters that were used to encode it.
``CONFIG_HEATSHRINK_DYNAMIC_ALLOC`` allows these parameters to be selected at runtime instead,
at the cost of requiring a heap. ``CONFIG_HEATSHRINK_ENCODER`` and ``CONFIG_HEATSHRINK_DECODER``
can be disabled individually to reduce the footprint.

Refer to the ``heatshrink_encoder.h`` and ``heatshrink_decoder.h`` headers, the
`heatshrink Zephyr module`_ documentation and the provided `heatshrink sample`_ for API details.

References
**********

.. target-notes::

.. _heatshrink:
   https://github.com/atomicobject/heatshrink

.. _heatshrink Zephyr module:
   https://github.com/kickmaker/heatshrink

.. _heatshrink sample:
   https://github.com/kickmaker/heatshrink/tree/zephyr/samples/compression
