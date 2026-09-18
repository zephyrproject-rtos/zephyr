.. _knx_application:

Writing a KNX application
##########################

The KNX stack itself only implements the *protocol*: framing, the Transport
Layer state machine, and the generic Interface Object / Property model ETS
uses to configure a device. It has no idea what a given product actually
*does* — how many sensors it has, what their Datapoint Types are, or what
their ETS parameters mean. That part is supplied by the application, as:

* a **Zephyr application** in the usual sense (``CMakeLists.txt``,
  ``prj.conf``, ``src/``, a board overlay), and
* one header, conventionally named ``knx_app_data.h``, declaring two
  structs that match exactly what the product's **ETS application
  program** declares.

Directory layout
*****************

.. code-block:: text

   my_knx_app/
   ├── CMakeLists.txt
   ├── prj.conf
   ├── west.yml
   ├── boards/
   │   └── <board>.overlay        # peripherals your product adds to the carrier board
   └── src/
       ├── main.c
       └── knx_app_data.h         # struct application_program_data / group_objects_data

Kconfig
*******

At minimum, ``prj.conf`` needs the stack, a Physical Layer driver, and the
sizing/identity options that tie the application's structs to the ETS
project:

.. code-block:: cfg

   CONFIG_KNX_STACK=y
   CONFIG_NCN5130=y            # or CONFIG_KNX_LOOPBACK=y for native_sim
   CONFIG_KNX_DPT=y

   # Match these to the <ApplicationProgram> element of the ETS product XML —
   # see "Generating knx_app_data.h from a .knxprod file" below.
   CONFIG_KNX_DECLARE_APPLICATION_PROGRAM=y
   CONFIG_KNX_APPLICATION_PROGRAM_ID=0x0000
   CONFIG_KNX_APPLICATION_PROGRAM_VERSION=0x01

   # Must be >= sizeof(struct application_program_data) / group_objects_data.
   CONFIG_KNX_APPLICATION_PROGRAM_DATA_SIZE=48
   CONFIG_KNX_GROUP_OBJECTS_DATA_SIZE=36
   CONFIG_KNX_GROUP_OBJECT_COUNT=9

See :ref:`knx` for the full Kconfig reference.

The ``knx_app_data.h`` contract
********************************

The public, stack-owned ``<zephyr/knx/knx_app_data.h>`` provides two macros
that turn a size mismatch between an application struct and its
Kconfig-reserved storage into a build failure instead of a silent buffer
overrun:

.. code-block:: c

   APPLICATION_PROGRAM_DATA(struct application_program_data);
   GROUP_OBJECTS_DATA(struct group_objects_data);

Call each once, anywhere the struct is visible (typically right after
including ``knx_app_data.h`` in ``main.c``). The application's own
``knx_app_data.h`` supplies the two struct definitions and the
``KNX_GO_*`` ASAP-number macros:

.. code-block:: c

   #include <zephyr/knx/dpt.h>
   #include <zephyr/knx/knx_app_data.h>

   struct application_program_data {
           float Scale1;
           float Offset1;
           /* ... one field per ETS parameter, in ETS memory-offset order ... */
   };

   struct group_objects_data {
           Type_DPT_Value_Temp Ch1_Temperature;
           Type_DPT_Enable     DebugMode;
           /* ... one field per ETS Group Object, in ASAP order ... */
   };

   #define KNX_GO_CH1_TEMPERATURE  1
   #define KNX_GO_DEBUG_MODE       9

``struct application_program_data`` mirrors the parameters ETS downloads
via ``A_Memory_Write`` (calibration values, thresholds, ...); its layout
must match the ETS project's parameter memory layout **exactly**, offset for
offset. ``struct group_objects_data`` is the live cache backing the
:ref:`Group Object API <knx_application_go_api>` below and is **not**
downloaded by ETS — only its *size* has to match what ETS's Group Object
Table declares.

Both structs, together with the ``KNX_GO_*`` macros, can be generated
mechanically from the ETS product file — see the next section — rather than
transcribed by hand from the ETS parameter/Group Object editor.

Generating ``knx_app_data.h`` from a ``.knxprod`` file
*********************************************************

ETS product databases are distributed as ``.knxprod`` files: a ZIP archive
of XML files describing one or more products' hardware, application
program, parameters and Group Objects. :zephyr_file:`scripts/utils/generate_header_from_knxprod.py`
reads one and prints a ready-to-use ``knx_app_data.h`` to stdout:

.. code-block:: bash

   zephyr/scripts/utils/generate_header_from_knxprod.py \
       my_knx_app/ets/my_product.knxprod > my_knx_app/src/knx_app_data.h

It walks the archive the same way ETS itself resolves a product:

.. code-block:: text

   <archive root>/<Manufacturer>/Hardware.xml
       -> <Hardware2Program><ApplicationProgramRef RefId="..."/>
   <archive root>/<Manufacturer>/<ApplicationProgramRef RefId>.xml
       -> <ApplicationProgram>
            <Static><Code><RelativeSegment Size="..."/></Code>   (-> application_program_data)
                    <Parameters>...</Parameters>                  (-> its fields, in ETS offset order)
                    <ComObjectTable>...</ComObjectTable>          (-> group_objects_data + KNX_GO_*)

The generated file's leading comment lists the :kconfig:option:`CONFIG_KNX_APPLICATION_PROGRAM_ID`,
:kconfig:option:`CONFIG_KNX_APPLICATION_PROGRAM_VERSION`,
:kconfig:option:`CONFIG_KNX_APPLICATION_PROGRAM_DATA_SIZE`,
:kconfig:option:`CONFIG_KNX_GROUP_OBJECT_COUNT` and
:kconfig:option:`CONFIG_KNX_GROUP_OBJECTS_DATA_SIZE` values ``prj.conf`` needs
to match it, computed straight from the ``.knxprod`` — copy them over rather
than re-deriving them by hand.

What it cannot know from the ``.knxprod`` alone
================================================

The Group Object entries in this archive format carry only a byte/bit size,
not a Datapoint Type number. The script guesses a Group Object's C cache
type (``Type_DPT_Value_Temp``, ``Type_DPT_Value_Volt``,
``Type_DPT_Enable``, ...) from keywords in its ETS name and falls back
to a same-width raw integer otherwise — every guess is marked with a
``/* TODO ... */`` comment in the generated struct and worth checking
against the product's real DPTs before shipping. Field and macro names are
also derived mechanically from the ETS Parameter/Group Object names, so
regenerating after renaming something in ETS will rename the corresponding
C identifier too — expect to update ``main.c`` accordingly, the same way
you would after editing the struct by hand.

Run ``generate_header_from_knxprod.py --help`` for the full list of
limitations and options (including ``--program-ref``, needed if
``Hardware.xml`` declares more than one hardware variant for the product).

Creating a ``.knxprod`` file
==============================

A ``.knxprod`` is normally produced by ETS's own Hardware/Application
editor, which requires a full ETS installation and license. The
`Kaenx-Creator <https://github.com/OpenKNX/Kaenx-Creator>`_ project, part of
the community `OpenKNX <https://github.com/OpenKNX>`_ effort, is an
alternative, open-source way to author the same ``.knxprod`` format —
defining the hardware, the application program's parameters and its Group
Objects — without needing ETS itself to produce the file that
``generate_header_from_knxprod.py`` consumes.

.. _knx_application_go_api:

Runtime Group Object API
***************************

Once the two structs above exist, the application drives its Group Objects
by ASAP number through ``knx_group_object.h`` — never by touching the
struct fields directly, so the storage layout can change without touching
call sites:

.. list-table::
   :header-rows: 1

   * - Function
     - Purpose
   * - ``knx_group_object_set_bool/u8/u16/i16/u32/i32/float16/float32(asap, value)``
     - Encode a new value into the cached buffer. No bus traffic.
   * - ``knx_group_object_get_bool/u8/u16/i16/u32/i32/float16/float32(asap)``
     - Decode the cached value. No bus traffic.
   * - ``knx_group_object_write(asap)``
     - Send the cached value as an ``A_GroupValue_Write`` telegram (only if
       the ETS Transmit flag is set on this Group Object).
   * - ``knx_group_object_set_update_handler(asap, cb)``
     - Register a callback fired when a Write telegram (or a response to a
       read we issued) updates the cache.
   * - ``knx_group_object_set_read_handler(asap, cb)``
     - Register a callback fired just before the stack auto-answers an
       incoming ``GroupValue_Read`` for this ASAP, so the application can
       refresh the cache first.

The stack never transmits a Group Object on its own initiative — an
application decides when to call ``knx_group_object_write()``, typically
after a periodic sample or a physical input change.

DPT encode/decode
*******************

.. doxygengroup:: knx_dpt

Building and flashing
************************

A KNX application builds like any other Zephyr application:

.. code-block:: bash

   west build -p always -b <board> my_knx_app/
   west flash
