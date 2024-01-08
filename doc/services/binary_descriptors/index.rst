.. _binary_descriptors:

Binary Descriptors
##################

Binary Descriptors are constant data objects storing information about the binary executable.
Unlike "regular" constants, binary descriptors are linked to a known offset in the binary, making
them accesible to other programs, such as a different image running on the same device or a host tool.
A few examples of constants that would make useful binary descriptors are: kernel version, app version,
build time, compiler version, environment variables, compiling host name, etc.

Binary descriptors are created by using the ``DEFINE_BINDESC_*`` macros. For example:

.. code-block:: c

   #include <zephyr/bindesc.h>

   BINDESC_STR_DEFINE(my_string, 2, "Hello world!"); // Unique ID is 2

``my_string`` could then be accessed using:

.. code-block:: c

   printk("my_string: %s\n", BINDESC_GET_STR(my_string));

But it could also be retrieved by ``west bindesc``:

.. code-block:: bash

   $ west bindesc custom_search STR 2 build/zephyr/zephyr.bin
   "Hello world!"

Internals
*********
Binary descriptors are implemented with a TLV (tag, length, value) header linked
to a known offset in the binary image. This offset may vary between architectures,
but generally the descriptors are linked as close to the beginning of the image as
possible. In architectures where the image must begin with a vector table (such as
ARM), the descriptors are linked right after the vector table. The reset vector points
to the beginning of the text section, which is after the descriptors. In architectures
where the image must begin with executable code (e.g. x86), a jump instruction is injected at
the beginning of the image, in order to skip over the binary descriptors, which are right
after the jump instruction.

Each tag is a 16 bit unsigned integer, where the most significant nibble (4 bits) is the type
(currently uint, string or bytes), and the rest is the ID. The ID is globally unique to each
descriptor. For example, the ID of the app version string is ``0x800``, and a string
is denoted by 0x1, making the app version tag ``0x1800``. The length is a 16 bit
number equal to the length of the data in bytes. The data is the actual descriptor
value. All binary descriptor numbers (magic, tags, uints) are laid out in memory
in the endianness native to the SoC. ``west bindesc`` assumes little endian by default,
so if the image belongs to a big endian SoC, the appropriate flag should be given to the
tool.

The binary descriptor header starts with the magic number ``0xb9863e5a7ea46046``. It's followed
by the TLVs, and ends with the ``DESCRIPTORS_END`` (``0xffff``) tag. The tags are
always aligned to 32 bits. If the value of the previous descriptor had a non-aligned
length, zero padding will be added to ensure that the current tag is aligned.

Putting it all together, here is what the example above would look like in memory
(of a little endian SoC):

.. code-block::

    46 60 a4 7e 5a 3e 86 b9 02 10  0d 00  48 65 6c 6c 6f 20 77 6f 72 6c 64 21 00 00 00 00 ff ff
   |         magic         | tag |length| H  e  l  l  o     w  o  r  l  d  !    |   pad  | end |

Usage
*****
Binary descriptors are always created by the ``BINDESC_*_DEFINE`` macros. As shown in
the example above, a descriptor can be generated from any string or integer, with any
ID. However, it is recommended to comply with the standard tags defined in
``include/zephyr/bindesc.h``, as that would have the following benefits:

 1. The ``west bindesc`` tool would be able to recognize what the descriptor means and
    print a meaningful tag
 2. It would enforce consistency between various apps from various sources
 3. It allows upstream-ability of descriptor generation (see Standard Descriptors)

To define a descriptor with a standard tag, just use the tags included from ``bindesc.h``:

.. code-block:: c

   #include <zephyr/bindesc.h>

   BINDESC_STR_DEFINE(app_version, BINDESC_ID_APP_VERSION_STRING, "1.2.3");

Standard Descriptors
====================
Some descriptors might be trivial to implement, and could therefore be implemented
in a standard way in upstream Zephyr. These could then be enabled via Kconfig, instead
of requiring every user to reimplement them. These include build times, kernel version,
and host info. For example, to add the build date and time as a string, the following
configs should be enabled:

.. code-block:: kconfig

   # Enable binary descriptors
   CONFIG_BINDESC=y

   # Enable definition of binary descriptors
   CONFIG_BINDESC_DEFINE=y

   # Enable default build time binary descriptors
   CONFIG_BINDESC_DEFINE_BUILD_TIME=y
   CONFIG_BINDESC_BUILD_DATE_TIME_STRING=y

To avoid collisions with user defined descriptors, the standard descriptors were alloted
the range between ``0x800-0xfff``. This leaves ``0x000-0x7ff`` to users.
For more information read the ``help`` sections of these Kconfig symbols.
By convention, each Kconfig symbol corresponds to a binary descriptor whose
name is the Kconfig name (with ``CONFIG_BINDESC_`` removed) in lower case. For example,
``CONFIG_BINDESC_KERNEL_VERSION_STRING`` creates a descriptor that can be
accessed using ``BINDESC_GET_STR(kernel_version_string)``.

west bindesc tool
=================
``west`` is able to parse and display binary descriptors from a given executable image.

For more information refer to ``west bindesc --help`` or the :ref:`documentation<west-bindesc>`.

API Reference
*************

.. doxygengroup:: bindesc_define
