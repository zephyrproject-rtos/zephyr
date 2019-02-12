.. _kconfig_tips_and_tricks:

Kconfig - Tips and Best Practices
#################################

.. contents::
    :local:
    :depth: 2

This page covers some Kconfig best practices and explains some Kconfig
behaviors and features that might be cryptic or that are easily overlooked.

.. note::

   The official Kconfig documentation is `kconfig-language.txt
   <https://raw.githubusercontent.com/torvalds/linux/master/Documentation/kbuild/kconfig-language.txt>`_
   and `kconfig-macro-language.txt
   <https://raw.githubusercontent.com/torvalds/linux/master/Documentation/kbuild/kconfig-macro-language.txt>`_.


What to turn into Kconfig options
*********************************

When deciding whether something belongs in Kconfig, it helps to distinguish
between symbols that have prompts and symbols that don't.

If a symbol has a prompt (e.g. ``bool "Enable foo"``), then the user can change
the symbol's value in the ``menuconfig`` interface (or by manually editing
configuration files). Therefore, only put a prompt on a symbol if it makes
sense for the user to change its value.

In Zephyr, Kconfig configuration is done after selecting a machine, so in
general, it does not make sense to put a prompt on a symbol that corresponds to
a fixed machine-specific setting. Usually, such settings should be handled via
device tree (``.dts``) files instead.

Symbols without prompts can't be configured directly by the user (they derive
their value from other symbols), so less restrictions apply to them. If some
derived setting is easier to calculate in Kconfig than e.g. during the build,
then do it in Kconfig, but keep the distinction between symbols with and
without prompts in mind.

See the `optional prompts`_ section for a way to deal with settings that are
fixed on some machines and configurable on other machines.


``select`` statements
****************************************

The ``select`` statement is used to force one symbol to ``y`` whenever another
symbol is ``y``. For example, the following code forces ``CONSOLE`` to ``y``
whenever ``USB_CONSOLE`` is ``y``:

.. code-block:: none

   config CONSOLE
   	bool "Console support"

   ...

   config USB_CONSOLE
   	bool "USB console support"
   	select CONSOLE

This section covers some pitfalls and good uses for ``select``.


``select`` pitfalls
===================

``select`` might seem like a generally useful feature at first, but can cause
configuration issues if overused.

For example, say that a new dependency is added to the ``CONSOLE`` symbol
above, by a developer who is unaware of the ``USB_CONSOLE`` symbol (or simply
forgot about it):

.. code-block:: none

   config CONSOLE
   	bool "Console support"
   	depends on STRING_ROUTINES

Enabling ``USB_CONSOLE`` now forces ``CONSOLE`` to ``y``, even if
``STRING_ROUTINES`` is ``n``.

To fix the problem, the ``STRING_ROUTINES`` dependency needs to be added to
``USB_CONSOLE`` as well:

.. code-block:: none

   config USB_CONSOLE
   	bool "USB console support"
   	select CONSOLE
   	depends on STRING_ROUTINES

   ...

   config STRING_ROUTINES
   	bool "Include string routines"

More insidious cases with dependencies inherited from ``if`` and ``menu``
statements are common.

An alternative attempt to solve the issue might be to turn the ``depends on``
into another ``select``:

.. code-block:: none

   config CONSOLE
   	bool "Console support"
   	select STRING_ROUTINES

   ...

   config USB_CONSOLE
   	bool "USB console support"
   	select CONSOLE

In practice, this often amplifies the problem, because any dependencies added
to ``STRING_ROUTINES`` now need to be copied to both ``CONSOLE`` and
``USB_CONSOLE``.

In general, whenever the dependencies of a symbol are updated, the dependencies
of all symbols that (directly or indirectly) select it have to be updated as
well. This is very often overlooked in practice, even for the simplest case
above.

Chains of symbols selecting each other should be avoided in particular, except
for simple helper symbols, as covered below in :ref:`good_select_use`.

Liberal use of ``select`` also tends to make Kconfig files harder to read, both
due to the extra dependencies and due to the non-local nature of ``select``,
which hides ways in which a symbol might get enabled.


Alternatives to ``select``
==========================

For the example in the previous section, a better solution is usually to turn
the ``select`` into a ``depends on``:

.. code-block:: none

   config CONSOLE
   	bool "Console support"

   ...

   config USB_CONSOLE
   	bool "USB console support"
   	depends on CONSOLE

This makes it impossible to generate an invalid configuration, and means that
dependencies only ever have to be updated in a single spot.

An objection to using ``depends on`` here might be that configuration files
that enable ``USB_CONSOLE`` now also need to enable ``CONSOLE``:

.. code-block:: none

   CONFIG_CONSOLE=y
   CONFIG_USB_CONSOLE=y

This comes down to a trade-off, but if enabling ``CONSOLE`` is the norm, then a
mitigation is to make ``CONSOLE`` default to ``y``:

.. code-block:: none

   config CONSOLE
   	bool "Console support"
   	default y

This gives just a single assignment in configuration files:

.. code-block:: none

   CONFIG_USB_CONSOLE=y

Note that configuration files that do not want ``CONSOLE`` enabled now have to
explicitly disable it:

.. code-block:: none

   CONFIG_CONSOLE=n


.. _good_select_use:

Using ``select`` for helper symbols
===================================

A good and safe use of ``select`` is for setting "helper" symbols that capture
some condition. Such helper symbols should preferably have no prompt or
dependencies.

For example, a helper symbol for indicating that a particular CPU/SoC has an
FPU could be defined as follows:

.. code-block:: none

   config CPU_HAS_FPU
   	bool
   	help
   	  If y, the CPU has an FPU

   ...

   config SOC_FOO
   	bool "FOO SoC"
   	select CPU_HAS_FPU

   ...

   config SOC_BAR
   	bool "BAR SoC"
   	select CPU_HAS_FPU

This makes it possible for other symbols to check for FPU support in a generic
way, without having to look for particular architectures:

.. code-block:: none

   config FLOAT
   	bool "Support floating point operations"
   	depends on CPU_HAS_FPU

The alternative would be to have dependencies like the following, possibly
duplicated in several spots:

.. code-block:: none

   config FLOAT
   	bool "Support floating point operations"
   	depends on SOC_FOO || SOC_BAR || ...

Invisible helper symbols can also be useful without ``select``. For example,
the following code defines a helper symbol that has the value ``y`` if the
machine has some arbitrarily-defined "large" amount of memory:

.. code-block:: none

   config LARGE_MEM
   	def_bool MEM_SIZE >= 64

.. note::

   This is short for the following:

   .. code-block:: none

      config LARGE_MEM
      	bool
      	default MEM_SIZE >= 64


``select`` recommendations
==========================

In summary, here are some recommended practices for ``select``:

- Avoid selecting symbols with prompts or dependencies. Prefer ``depends on``.
  If ``depends on`` causes annoying bloat in configuration files, consider
  adding a Kconfig default for the most common value.

  Rare exceptions might include cases where you're sure that the dependencies
  of the selecting and selected symbol will never drift out of sync, e.g. when
  dealing with two simple symbols defined close to one another within the same
  ``if``.

  Common sense applies, but be aware that ``select`` often causes issues in
  practice. ``depends on`` is usually a cleaner and safer solution.

- Select simple helper symbols without prompts and dependencies however much
  you like. They're a great tool for simplifying Kconfig files.


Style recommendations and shorthands
************************************

This section gives some style recommendations and explains some common Kconfig
shorthands.


Factoring out common dependencies
=================================

If a sequence of symbols/choices share a common dependency, the dependency can
be factored out with an ``if``.

As an example, consider the following code:

.. code-block:: none

   config FOO
   	bool "Foo"
   	depends on DEP

   config BAR
   	bool "Bar"
   	depends on DEP

   choice
   	prompt "Choice"
   	depends on DEP

   config BAZ
   	bool "Baz"

   config QAZ
   	bool "Qaz"

   endchoice

Here, the ``DEP`` dependency can be factored out like this:

.. code-block:: none

   if DEP

   config FOO
   	bool "Foo"

   config BAR
   	bool "Bar"

   choice
   	prompt "Choice"

   config BAZ
   	bool "Baz"

   config QAZ
   	bool "Qaz"

   endchoice

   endif # DEP

.. note::

   Internally, the second version of the code is transformed into the first.

If a sequence of symbols/choices with shared dependencies are all in the same
menu, the dependency can be put on the menu itself:

.. code-block:: none

   menu "Foo features"
   	depends on FOO_SUPPORT

   config FOO_FEATURE_1
   	bool "Foo feature 1"

   config FOO_FEATURE_2
   	bool "Foo feature 2"

   endmenu

If ``FOO_SUPPORT`` is ``n``, the entire menu disappears.


Redundant defaults
==================

``bool`` symbols implicitly default to ``n``, and ``string`` symbols implicitly
default to the empty string. Therefore, ``default n`` and ``default ""`` are
(almost) always redundant.

The recommended style in Zephyr is to skip redundant defaults for ``bool`` and
``string`` symbols. That also generates clearer documentation: (*Implicitly
defaults to n* instead of *n if <dependencies, possibly inherited>*).

.. note::

   The one case where ``default n``/``default ""`` is not redundant is when
   defining a symbol in multiple locations and wanting to override e.g. a
   ``default y`` on a later definition.

Defaults *should* always be given for ``int`` and ``hex`` symbols, however, as
they implicitly default to the empty string. This is partly for compatibility
with the C Kconfig tools, though an implicit 0 default might be less likely to
be what was intended compared to other symbol types as well.


Common shorthands
=================

Kconfig has two shorthands that deal with prompts and defaults.

- ``<type> "prompt"`` is a shorthand for giving a symbol/choice a type and a
  prompt at the same time. These two definitions are equal:

  .. code-block:: none

     config FOO
     	bool "foo"

  .. code-block:: none

     config FOO
     	bool
     	prompt "foo"

  The first style, with the shorthand, is preferred in Zephyr.

- ``def_<type> <value>`` is a shorthand for giving a type and a value at the
  same time. These two definitions are equal:

  .. code-block:: none

     config FOO
     	def_bool BAR && BAZ

  .. code-block:: none

     config FOO
     	bool
     	default BAR && BAZ

Using both the ``<type> "prompt"`` and the ``def_<type> <value>`` shorthand in
the same definition is redundant, since it gives the type twice.

The ``def_<type> <value>`` shorthand is generally only useful for symbols
without prompts, and somewhat obscure.

.. note::

   For a symbol defined in multiple locations (e.g., in a ``Kconfig.defconfig``
   file in Zephyr), it is best to only give the symbol type for the "base"
   definition of the symbol, and to use ``default`` (instead of ``def_<type>
   value``) for the remaining definitions. That way, if the base definition of
   the symbol is removed, the symbol ends up without a type, which generates a
   warning that points to the other definitions. That makes the extra
   definitions easier to discover and remove.


Lesser-known/used Kconfig features
**********************************

This section lists some more obscure Kconfig behaviors and features that might
still come in handy.


The ``imply`` statement
=======================

The ``imply`` statement is similar to ``select``, but respects dependencies and
doesn't force a value. For example, the following code could be used to enable
USB keyboard support by default on the FOO SoC, while still allowing the user
to turn it off:

.. code-block:: none

   config SOC_FOO
   	bool "FOO SoC"
   	imply USB_KEYBOARD

   ...

   config USB_KEYBOARD
   	bool "USB keyboard support"

``imply`` acts like a suggestion, whereas ``select`` forces a value.


Optional prompts
================

A condition can be put on a symbol's prompt to make it optionally configurable
by the user. For example, a value ``MASK`` that's hardcoded to 0xFF on some
boards and configurable on others could be expressed as follows:

.. code-block:: none

   config MASK
   	hex "Bitmask" if HAS_CONFIGURABLE_MASK
   	default 0xFF

.. note::

   This is short for the following:

   .. code-block:: none

      config MASK
      	hex
      	prompt "Bitmask" if HAS_CONFIGURABLE_MASK
      	default 0xFF

The ``HAS_CONFIGURABLE_MASK`` helper symbol would get selected by boards to
indicate that ``MASK`` is configurable. When ``MASK`` is configurable, it will
also default to 0xFF.


Optional choices
================

Defining a choice with the ``optional`` keyword allows the whole choice to be
toggled off to select none of the symbols:

.. code-block:: none

   choice
   	prompt "Use legacy protocol"
   	optional

   config LEGACY_PROTOCOL_1
   	bool "Legacy protocol 1"

   config LEGACY_PROTOCOL_2
   	bool "Legacy protocol 2"

   endchoice

In the menuconfig interface, this will be displayed e.g. as ``[*] Use legacy
protocol (LEGACY_PROTOCOL_1) --->``, where the choice can be toggled off to
enable neither of the symbols.


``visible if`` conditions
=========================

Putting a ``visible if`` condition on a menu hides the menu and all the symbols
within it, while still allowing symbol default values to kick in.

As a motivating example, consider the following code:

.. code-block:: none

   menu "Foo subsystem"
   	depends on HAS_CONFIGURABLE_FOO

   config FOO_SETTING_1
   	int "Foo setting 1"
   	default 1

   config FOO_SETTING_2
   	int "Foo setting 2"
   	default 2

   endmenu

When ``HAS_CONFIGURABLE_FOO`` is ``n``, no configuration output is generated
for ``FOO_SETTING_1`` and ``FOO_SETTING_2``, as the code above is logically
equivalent to the following code:

.. code-block:: none

   config FOO_SETTING_1
   	int "Foo setting 1"
   	default 1
   	depends on HAS_CONFIGURABLE_FOO

   config FOO_SETTING_2
   	int "Foo setting 2"
   	default 2
   	depends on HAS_CONFIGURABLE_FOO

If we want the symbols to still get their default values even when
``HAS_CONFIGURABLE_FOO`` is ``n``, but not be configurable by the user, then we
can use ``visible if`` instead:

.. code-block:: none

   menu "Foo subsystem"
   	visible if HAS_CONFIGURABLE_FOO

   config FOO_SETTING_1
   	int "Foo setting 1"
   	default 1

   config FOO_SETTING_2
   	int "Foo setting 2"
   	default 2

   endmenu

This is logically equivalent to the following:

.. code-block:: none

   config FOO_SETTING_1
   	int "Foo setting 1" if HAS_CONFIGURABLE_FOO
   	default 1

   config FOO_SETTING_2
   	int "Foo setting 2" if HAS_CONFIGURABLE_FOO
   	default 2

.. note::

   See the `optional prompts`_ section for the meaning of the conditions on the
   prompts.

When ``HAS_CONFIGURABLE`` is ``n``, we now get the following configuration
output for the symbols, instead of no output:

.. code-block:: none

   ...
   CONFIG_FOO_SETTING_1=1
   CONFIG_FOO_SETTING_2=2
   ...


.. _kconfig-functions:

Kconfig Functions
*****************

Kconfiglib provides user-defined preprocessor functions that
we use in Zephyr to expose Device Tree information to Kconfig.
For example, we can get the default value for a Kconfig symbol
from the device tree.

Device Tree Related Functions
=============================

.. code-block:: none

  dt_int_val(kconf, _, name, unit):
       This function looks up 'name' in the DTS generated "conf" style database
       (generated_dts_board.conf in <build_dir>/zephyr/include/generated/)
       and if it's found it will return the value as an decimal integer.  The
       function will divide the value based on 'unit':
           None        No division
           'k' or 'K'  divide by 1024 (1 << 10)
           'm' or 'M'  divide by 1,048,576 (1 << 20)
           'g' or 'G'  divide by 1,073,741,824 (1 << 30)

  dt_hex_val(kconf, _, name, unit):
       This function looks up 'name' in the DTS generated "conf" style database
       (generated_dts_board.conf in <build_dir>/zephyr/include/generated/)
       and if it's found it will return the value as an hex integer.  The
       function will divide the value based on 'unit':
           None        No division
           'k' or 'K'  divide by 1024 (1 << 10)
           'm' or 'M'  divide by 1,048,576 (1 << 20)
           'g' or 'G'  divide by 1,073,741,824 (1 << 30)

  dt_str_val(kconf, _, name):
       This function looks up 'name' in the DTS generated "conf" style database
       (generated_dts_board.conf in <build_dir>/zephyr/include/generated/)
       and if it's found it will return the value as string. if it's not found we
       return an empty string.

Example Usage
=============

The following example shows the usage of the ``dt_int_val`` function:

.. code-block:: none

   boards/arm/mimxrt1020_evk/Kconfig.defconfig

   config FLASH_SIZE
      default $(dt_int_val,DT_NXP_IMX_FLEXSPI_402A8000_SIZE_1,K)

In this example if we examine the generated generated_dts_board.conf file
as part of the Zephyr build we'd find the following entry:

.. code-block:: none

   DT_NXP_IMX_FLEXSPI_402A8000_SIZE_1=8388608

The ``dt_int_val`` will search the generated_dts_board.conf that is derived from
the dts for the board and match the ``DT_NXP_IMX_FLEXSPI_402A8000_SIZE_1`` entry.
The function than will than scale the value by ``1024``.  This effective causes
the above to look like:

.. code-block:: none

   config FLASH_SIZE
      default 8192


Other resources
***************

The *Intro to symbol values* section in the `Kconfiglib docstring
<https://github.com/ulfalizer/Kconfiglib/blob/master/kconfiglib.py>`_ goes over
how symbols values are calculated in more detail.
