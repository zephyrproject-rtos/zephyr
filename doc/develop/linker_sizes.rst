Linker section sizes
####################

Zephyr linker scripts provide section boundary and size symbols. A size symbol
such as ``__tdata_size`` is an absolute value, not an object at that address.
Loading it as a C address can produce a PC-relative relocation that exceeds
the range supported by the selected code model.

Linker section sizes are therefore accessed according to where the value is
needed:

* Runtime code calculates a size from an address extent.
* Link-time constant expressions read a linker-provided size symbol.

Runtime sizes
*************

Use ``LINKER_SECTION_SIZE`` with two boundaries in the same address domain:

.. code-block:: c

   #include <zephyr/linker/linker-defs.h>

   size_t size =
       LINKER_SECTION_SIZE(__tdata_start, __tdata_end);

The limit is exclusive and both symbols must use the same address domain.
For example, do not mix a load address from Flash with a link address in RAM.
The linker script must place alignment padding inside the extent when the
consumer requires it.

When a section has no natural end symbol, the linker script or linker
generator can provide one:

.. code-block:: ld

   _foo_limit = _foo_start + _foo_size;

The consumer then uses:

.. code-block:: c

   size_t size = LINKER_SECTION_SIZE(_foo_start, _foo_limit);

A synthetic limit is a measurement endpoint and does not imply that the range
is physically contiguous.

Complex sizes
*************

Some logical sizes are composed from multiple address extents. Calculate and
combine those extents explicitly:

.. code-block:: c

   size_t size =
       LINKER_SECTION_SIZE(_image_ram_start, _image_ram_end) +
       LINKER_SECTION_SIZE(_iram_start, _iram_end);

TLS is another composite case. Stack canary, initialized TLS data, and TLS BSS
must be copied or cleared separately because the linker can insert padding
between them.

Link-time constants
*******************

Static initializers and other constant expressions cannot use a runtime
subtraction. Use ``LINKER_SECTION_SIZE_CONST`` for those cases:

.. code-block:: c

   static const struct region regions[] = {
       {
           .size = LINKER_SECTION_SIZE_CONST(__gcov_bss_size),
       },
   };

The size symbol remains available for compatibility and for linker-generated
constants. It must not be used as the runtime source of a section length.

Some external toolchains provide only a size symbol, without a matching
boundary, for example ARC MWDT TLS. Those symbols remain toolchain-specific
until the generator can also provide an extent. They must not be used as the
model for a general Zephyr runtime API.

Rules for contributors
**********************

#. Use ``LINKER_SECTION_SIZE(start, limit)`` in runtime code.
#. Use ``LINKER_SECTION_SIZE_CONST(size_symbol)`` only where C requires a
   link-time constant.
#. Choose a start and limit from the same address domain.
#. Include alignment and hardware minimum padding in the extent.
#. Do not model multiple discontinuous ranges as one physical extent. Combine
   their sizes explicitly.
#. Keep size symbols for static contexts and compatibility; do not treat them
   as a universal runtime interface.

The linker script is the source of truth for placement, alignment, and
boundaries. Consumer code derives runtime lengths from those boundaries.
