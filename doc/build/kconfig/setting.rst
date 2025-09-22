.. _setting_configuration_values:

Setting Kconfig configuration values
####################################

The :ref:`menuconfig and guiconfig interfaces <menuconfig>` can be used to test
out configurations during application development. This page explains how to
make settings permanent.

All Kconfig options can be searched in the :ref:`Kconfig search page
<kconfig-search>`.

.. note::

   Before making changes to Kconfig files, it's a good idea to also go through
   the :ref:`kconfig_tips_and_tricks` page.


Visible and invisible Kconfig symbols
*************************************

When making Kconfig changes, it's important to understand the difference
between *visible* and *invisible* symbols.

- A visible symbol is a symbol defined with a prompt. Visible symbols show
  up in the interactive configuration interfaces (hence *visible*), and can be
  set in configuration files.

  Here's an example of a visible symbol:

  .. code-block:: kconfig

     config FPU
     	bool "Support floating point operations"
     	depends on HAS_FPU

  The symbol is shown like this in ``menuconfig``, where it can be toggled:

  .. code-block:: none

     [ ] Support floating point operations

- An *invisible* symbol is a symbol without a prompt. Invisible symbols are
  not shown in the interactive configuration interfaces, and users have no
  direct control over their value. They instead get their value from defaults
  or from other symbols.

  Here's an example of an invisible symbol:

  .. code-block:: kconfig

     config CPU_HAS_FPU
     	bool
     	help
     	  This symbol is y if the CPU has a hardware floating point unit.

  In this case, ``CPU_HAS_FPU`` is enabled through other symbols having
  ``select CPU_HAS_FPU``.


Setting symbols in configuration files
**************************************

Visible symbols can be configured by setting them in configuration files. The
initial configuration is produced by merging a :file:`*_defconfig` file for the
board with application settings, usually from :file:`prj.conf`. See
:ref:`initial-conf` below for more details.

Assignments in configuration files use this syntax:

.. code-block:: cfg

   CONFIG_<symbol name>=<value>

There should be no spaces around the equals sign.

``bool`` symbols can be enabled or disabled by setting them to ``y`` or ``n``,
respectively. The ``FPU`` symbol from the example above could be enabled like
this:

.. code-block:: cfg

   CONFIG_FPU=y

.. note::

   A boolean symbol can also be set to ``n`` with a comment formatted like
   this:

   .. code-block:: cfg

      # CONFIG_SOME_OTHER_BOOL is not set

   This is the format you will see in the merged configuration
   saved to :file:`zephyr/.config` in the build directory.

   This style is accepted for historical reasons: Kconfig configuration files
   can be parsed as makefiles (though Zephyr doesn't use this). Having
   ``n``-valued symbols correspond to unset variables simplifies tests in Make.

Other symbol types are assigned like this:

.. code-block:: cfg

   CONFIG_SOME_STRING="cool value"
   CONFIG_SOME_INT=123

Comments use a #:

.. code-block:: cfg

   # This is a comment

Assignments in configuration files are only respected if the dependencies for
the symbol are satisfied. A warning is printed otherwise. To figure out what
the dependencies of a symbol are, use one of the :ref:`interactive
configuration interfaces <menuconfig>` (you can jump directly to a symbol with
:kbd:`/`), or look up the symbol in the :ref:`Kconfig search page
<kconfig-search>`.


.. _initial-conf:

The Initial Configuration
*************************

The initial configuration for an application comes from merging configuration
settings from three sources:

1. A ``BOARD``-specific configuration file stored in
   :file:`boards/<VENDOR>/<BOARD>/<BOARD>_defconfig`

2. Any CMake cache entries prefix with ``CONFIG_``

3. The application configuration

The application configuration can come from the sources below (each file is
known as a Kconfig fragment, which are then merged to get the final
configuration used for a particular build). By default, :file:`prj.conf` is
used.

#. If ``CONF_FILE`` is set, the configuration file(s) specified in it are
   merged and used as the application configuration. ``CONF_FILE`` can be set
   in various ways:

   1. In :file:`CMakeLists.txt`, before calling ``find_package(Zephyr)``

   2. By passing ``-DCONF_FILE=<conf file(s)>``, either directly or via ``west``

   3. From the CMake variable cache

#. Otherwise, if :file:`boards/<BOARD>.conf` exists in the application
   configuration directory, the result of merging it with :file:`prj.conf` is
   used.

#. Otherwise, if board revisions are used and
   :file:`boards/<BOARD>_<revision>.conf` exists in the application
   configuration directory, the result of merging it with :file:`prj.conf` and
   :file:`boards/<BOARD>.conf` is used.

#. Otherwise, :file:`prj.conf` is used from the application configuration
   directory. If it does not exist then a fatal error will be emitted.

Furthermore, applications can have SoC overlay configuration that is applied to
it, the file :file:`socs/<SOC>_<BOARD_QUALIFIERS>.conf` will be applied if it exists,
after the main project configuration has been applied and before any board overlay
configuration files have been applied.

All configuration files will be taken from the application's configuration
directory except for files with an absolute path that are given with the
``CONF_FILE``, ``EXTRA_CONF_FILE``, ``DTC_OVERLAY_FILE``, and
``EXTRA_DTC_OVERLAY_FILE`` arguments.  For these,
a file in a Zephyr module can be referred by escaping the Zephyr module dir
variable like this ``\${ZEPHYR_<module>_MODULE_DIR}/<path-to>/<file>``
when setting any of said variables in the application's :file:`CMakeLists.txt`.

See :ref:`Application Configuration Directory <application-configuration-directory>`
on how the application configuration directory is defined.

If a symbol is assigned both in :file:`<BOARD>_defconfig` and in the
application configuration, the value set in the application configuration takes
precedence.

The merged configuration is saved to :file:`zephyr/.config` in the build
directory.

As long as :file:`zephyr/.config` exists and is up-to-date (is newer than any
``BOARD`` and application configuration files), it will be used in preference
to producing a new merged configuration. :file:`zephyr/.config` is also the
configuration that gets modified when making changes in the :ref:`interactive
configuration interfaces <menuconfig>`.


Tracking Kconfig symbols
************************

It is possible to create Kconfig symbols which takes the default value of
another Kconfig symbol.

This is valuable when you want a symbol specific to an application or subsystem
but do not want to rely directly on the common symbol. For example, you might
want to decouple the settings so they can be independently configured, or to
ensure you always have a locally named setting, even if the external setting name changes.
is later changed.

For example, consider the common ``FOO_STRING`` setting where a subsystem wants
to have a ``SUB_FOO_STRING`` but still allow for customization.

This can be done like this:

.. code-block:: kconfig

    config FOO_STRING
            string "Foo"
            default "foo"

    config SUB_FOO_STRING
            string "Sub-foo"
            default FOO_STRING

This ensures that the default value of ``SUB_FOO_STRING`` is identical to
``FOO_STRING`` while still allows users to configure both settings
independently.

It is also possible to make ``SUB_FOO_STRING`` invisible and thereby keep the
two symbols in sync, unless the value of the tracking symbol is changed in a
:file:`defconfig` file.

.. code-block:: kconfig

    config FOO_STRING
            string "Foo"
            default "foo"

    config SUB_FOO_STRING
            string
            default FOO_STRING
            help
              Hidden symbol which follows FOO_STRING
              Can be changed through *.defconfig files.


Configuring invisible Kconfig symbols
*************************************

When making changes to the default configuration for a board, you might have to
configure invisible symbols. This is done in
:file:`boards/<VENDOR>/<BOARD>/Kconfig.defconfig`, which is a regular
:file:`Kconfig` file.

.. note::

    Assignments in :file:`.config` files have no effect on invisible symbols,
    so this scheme is not just an organizational issue.

Assigning values in :file:`Kconfig.defconfig` relies on defining a Kconfig
symbol in multiple locations. As an example, say we want to set ``FOO_WIDTH``
below to 32:

.. code-block:: kconfig

    config FOO_WIDTH
    	int

To do this, we extend the definition of ``FOO_WIDTH`` as follows, in
:file:`Kconfig.defconfig`:

.. code-block:: kconfig

    if BOARD_MY_BOARD

    config FOO_WIDTH
    	default 32

    endif

.. note::

   Since the type of the symbol (``int``) has already been given at the first
   definition location, it does not need to be repeated here. Only giving the
   type once at the "base" definition of the symbol is a good idea for reasons
   explained in :ref:`kconfig_shorthands`.

``default`` values in :file:`Kconfig.defconfig` files have priority over
``default`` values given on the "base" definition of a symbol. Internally, this
is implemented by including the :file:`Kconfig.defconfig` files first. Kconfig
uses the first ``default`` with a satisfied condition, where an empty condition
corresponds to ``if y`` (is always satisfied).

Note that conditions from surrounding top-level ``if``\ s are propagated to
symbol properties, so the above ``default`` is equivalent to
``default 32 if BOARD_MY_BOARD``.

.. _multiple_symbol_definitions:

Multiple symbol definitions
---------------------------

When a symbol is defined in multiple locations, each definition acts as an
independent symbol that happens to share the same name. This means that
properties are not appended to previous definitions. If the conditions
for **ANY** definition result in the symbol resolving to ``y``, the symbol
will be ``y``. It is therefore not possible to make the dependencies of a
symbol more restrictive by defining it in multiple locations.

For example, the dependencies of the symbol ``FOO`` below are satisfied if
either ``DEP1`` **OR** ``DEP2`` are true, it does not require both:

.. code-block:: none

   config FOO
      ...
      depends on DEP1

   config FOO
      ...
      depends on DEP2

.. warning::
   Symbols without explicit dependencies still follow the above rule. A
   symbol without any dependencies will result in the symbol always being
   assignable. The definition below will result in ``FOO`` always being
   enabled by default, regardless of the value of ``DEP1``.

   .. code-block:: kconfig

      config FOO
         bool "FOO"
         depends on DEP1

      config FOO
         default y

   This dependency weakening can be avoided with the :ref:`configdefault
   <kconfig_extensions>` extension if the desire is only to add a new default
   without modifying any other behaviour of the symbol.

.. note::
   When making changes to :file:`Kconfig.defconfig` files, always check the
   symbol's direct dependencies in one of the :ref:`interactive configuration
   interfaces <menuconfig>` afterwards. It is often necessary to repeat
   dependencies from the base definition of the symbol to avoid weakening a
   symbol's dependencies.


Motivation for Kconfig.defconfig files
--------------------------------------

One motivation for this configuration scheme is to avoid making fixed
``BOARD``-specific settings configurable in the interactive configuration
interfaces. If all board configuration were done via :file:`<BOARD>_defconfig`,
all symbols would have to be visible, as values given in
:file:`<BOARD>_defconfig` have no effect on invisible symbols.

Having fixed settings be user-configurable would clutter up the configuration
interfaces and make them harder to understand, and would make it easier to
accidentally create broken configurations.

When dealing with fixed board-specific settings, also consider whether they
should be handled via :ref:`devicetree <dt-guide>` instead.


Configuring choices
-------------------

There are two ways to configure a Kconfig ``choice``:

1. By setting one of the choice symbols to ``y`` in a configuration file.

   Setting one choice symbol to ``y`` automatically gives all other choice
   symbols the value ``n``.

   If multiple choice symbols are set to ``y``, only the last one set to ``y``
   will be honored (the rest will get the value ``n``). This allows a choice
   selection from a board :file:`defconfig` file to be overridden from an
   application :file:`prj.conf` file.

2. By changing the ``default`` of the choice in :file:`Kconfig.defconfig`.

   As with symbols, changing the default for a choice is done by defining the
   choice in multiple locations. For this to work, the choice must have a name.

   As an example, assume that a choice has the following base definition (here,
   the name of the choice is ``FOO``):

   .. code-block:: kconfig

       choice FOO
           bool "Foo choice"
           default B

       config A
           bool "A"

       config B
           bool "B"

       endchoice

   To change the default symbol of ``FOO`` to ``A``, you would add the
   following definition to :file:`Kconfig.defconfig`:

   .. code-block:: kconfig

       choice FOO
           default A
       endchoice

The :file:`Kconfig.defconfig` method should be used when the dependencies of
the choice might not be satisfied. In that case, you're setting the default
selection whenever the user makes the choice visible.


More Kconfig resources
======================

The :ref:`kconfig_tips_and_tricks` page has some tips for writing Kconfig
files.

The :zephyr_file:`kconfiglib.py <scripts/kconfig/kconfiglib.py>` docstring
(at the top of the file) goes over how symbol values are calculated in detail.
