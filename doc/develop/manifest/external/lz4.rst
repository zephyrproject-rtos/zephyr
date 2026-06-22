.. _external_module_lz4:

LZ4 - Extremely fast compression
################################

Introduction
************

LZ4 is a lossless compression algorithm, providing compression speed greater
than 500 MB/s per core, scalable with multi-core CPUs. It features an
extremely fast decoder, with speed in multiple GB/s per core, typically
reaching RAM speed limits on multi-core systems.

Speed can be tuned dynamically by selecting an "acceleration" factor, which
trades compression ratio for faster speed. On the other end, a high
compression derivative, LZ4_HC, is also provided, trading CPU time for
improved compression ratio. All versions feature the same decompression speed.

LZ4 is also compatible with dictionary compression, both at API and CLI
levels. It can ingest any input file as a dictionary, though only the final
64KB are used. This capability can be combined


Usage with Zephyr
*****************

To pull in lz4 as a Zephyr module, either add it as a West project in the ``west.yaml``
file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/lz4.yaml``) file
with the following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: lz4
         url: https://github.com/zephyrproject-rtos/lz4
         revision: zephyr
         path: modules/lib/lz4 # adjust the path as needed

For more detailed instructions and API documentation, refer to the `lz4 documentation`_ as
well as the provided `lz4 examples`_.


Reference
*********

.. _lz4 documentation:
    https://github.com/lz4/lz4/tree/dev/doc

.. _lz4 examples:
   https://github.com/zephyrproject-rtos/lz4/tree/zephyr/zephyr/samples
