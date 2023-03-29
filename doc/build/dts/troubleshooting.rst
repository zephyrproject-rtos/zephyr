.. _dt-trouble:

Troubleshooting devicetree
##########################

Here are some tips for fixing misbehaving devicetree related code.

See :ref:`dt-howtos` for other "HOWTO" style information.

Try again with a pristine build directory
*****************************************

.. important:: Try this first, before doing anything else.

See :ref:`west-building-pristine` for examples, or just delete the build
directory completely and retry.

This is general advice which is especially applicable to debugging devicetree
issues, because the outputs are created during the CMake configuration phase,
and are not always regenerated when one of their inputs changes.

Make sure <devicetree.h> is included
************************************

Unlike Kconfig symbols, the :file:`devicetree.h` header must be included
explicitly.

Many Zephyr header files rely on information from devicetree, so including some
other API may transitively include :file:`devicetree.h`, but that's not
guaranteed.

.. _dt-use-the-right-names:

Make sure you're using the right names
**************************************

Remember that:

- In C/C++, devicetree names must be lowercased and special characters must be
  converted to underscores. Zephyr's generated devicetree header has DTS names
  converted in this way into the C tokens used by the preprocessor-based
  ``<devicetree.h>`` API.
- In overlays, use devicetree node and property names the same way they
  would appear in any DTS file. Zephyr overlays are just DTS fragments.

For example, if you're trying to **get** the ``clock-frequency`` property of a
node with path ``/soc/i2c@12340000`` in a C/C++ file:

.. code-block:: c

   /*
    * foo.c: lowercase-and-underscores names
    */

   /* Don't do this: */
   #define MY_CLOCK_FREQ DT_PROP(DT_PATH(soc, i2c@1234000), clock-frequency)
   /*                                           ^               ^
    *                                        @ should be _     - should be _  */

   /* Do this instead: */
   #define MY_CLOCK_FREQ DT_PROP(DT_PATH(soc, i2c_1234000), clock_frequency)
   /*                                           ^               ^           */

And if you're trying to **set** that property in a devicetree overlay:

.. code-block:: none

   /*
    * foo.overlay: DTS names with special characters, etc.
    */

   /* Don't do this; you'll get devicetree errors. */
   &{/soc/i2c_12340000/} {
   	clock_frequency = <115200>;
   };

   /* Do this instead. Overlays are just DTS fragments. */
   &{/soc/i2c@12340000/} {
   	clock-frequency = <115200>;
   };

Look at the preprocessor output
*******************************

To save preprocessor output files, enable the
:kconfig:option:`CONFIG_COMPILER_SAVE_TEMPS` option. For example, to build
:ref:`hello_world` with west with this option set, use:

.. code-block:: sh

   west build -b BOARD samples/hello_world -- -DCONFIG_COMPILER_SAVE_TEMPS=y

This will create a preprocessor output file named :file:`foo.c.i` in the build
directory for each source file :file:`foo.c`.

You can then search for the file in the build directory to see what your
devicetree macros expanded to. For example, on macOS and Linux, using ``find``
to find :file:`main.c.i`:

.. code-block:: sh

   $ find build -name main.c.i
   build/CMakeFiles/app.dir/src/main.c.i

It's usually easiest to run a style formatter on the results before opening
them. For example, to use ``clang-format`` to reformat the file in place:

.. code-block:: sh

   clang-format -i build/CMakeFiles/app.dir/src/main.c.i

You can then open the file in your favorite editor to view the final C results
after preprocessing.

Validate properties
*******************

If you're getting a compile error reading a node property, check your node
identifier and property. For example, if you get a build error on a line that
looks like this:

.. code-block:: c

   int baud_rate = DT_PROP(DT_NODELABEL(my_serial), current_speed);

Try checking the node by adding this to the file and recompiling:

.. code-block:: c

   #if !DT_NODE_EXISTS(DT_NODELABEL(my_serial))
   #error "whoops"
   #endif

If you see the "whoops" error message when you rebuild, the node identifier
isn't referring to a valid node. :ref:`get-devicetree-outputs` and debug from
there.

Some hints for what to check next if you don't see the "whoops" error message:

- did you :ref:`dt-use-the-right-names`?
- does the :ref:`property exist <dt-checking-property-exists>`?
- does the node have a :ref:`matching binding <dt-bindings>`?
- does the binding define the property?

.. _missing-dt-binding:

Check for missing bindings
**************************

See :ref:`dt-bindings` for information about bindings, and
:ref:`devicetree_binding_index` for information on bindings built into Zephyr.

If the build fails to :ref:`dts-find-binding` for a node, then either the
node's ``compatible`` property is not defined, or its value has no matching
binding. If the property is set, check for typos in its name. In a devicetree
source file, ``compatible`` should look like ``"vnd,some-device"`` --
:ref:`dt-use-the-right-names`.

If your binding file is not under :file:`zephyr/dts`, you may need to set
:ref:`DTS_ROOT <dts_root>`; see :ref:`dt-where-bindings-are-located`.

Errors with DT_INST_() APIs
***************************

If you're using an API like :c:func:`DT_INST_PROP`, you must define
``DT_DRV_COMPAT`` to the lowercase-and-underscores version of the compatible
you are interested in. See :ref:`dt-create-devices-inst`.
