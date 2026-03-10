.. _kconfig_traceconfig:

Tracing values to their source
##############################

The merged configuration saved to :file:`zephyr/.config` contains the result of
applying the user configuration file to the whole tree of Kconfig files. This
process can be hard to follow, especially when invisible symbols are being
implicitly selected from anywhere in the tree. To help with this, the
``traceconfig`` target can be used to generate a file in the build directory
that details how each symbol got its final value.

After building the Zephyr project as usual, the Kconfig trace can be generated
with either of these commands:

   .. code-block:: bash

      west build -t traceconfig

   .. code-block:: bash

      ninja traceconfig

.. note::
   The generated information is only useful on a clean build, because otherwise
   the ``.config`` file "pins" all settings to a specific value (as described
   in :ref:`Stuck symbols <stuck_symbols>`). Therefore, it is recommended to
   run a :ref:`pristine build <west-building-pristine>` before generating the
   trace.

The output will be in the :file:`zephyr/kconfig-trace.md` file in the build
directory. This file is best viewed within IDEs to take advantage of Markdown
elements (tables, highlighting, clickable links), but can easily be understood
even when opened directly with any text editor.

The report is divided in three sections:

#. Visible symbols
#. Invisible symbols
#. Unset symbols

For sections 1 and 2, a table is presented where each symbol is shown with its
type, name, and current value. The fourth column details the kind of statement
that resulted in the value being applied, and can be one of the following:

 - *assigned*, when an explicit assignment, of the form ``CONFIG_xxx=y``, is
   read from a config file;

 - *default*, when there was no user assignment, but an applicable default
   value was read from Kconfig tree;

 - *selected* or *implied*, when a ``select`` or ``imply`` statement from a
   separate symbol caused the symbol to be set.

The fifth column details the location for the source statement. For the first
two kinds of statements, the exact location (file name and line number) is
provided; in the latter cases, it contains the expressions that resulted in the
symbol being set.

Finally, section 3 simply lists all undefined symbols. These have no additional
information since they never received a value by any of the above means.
