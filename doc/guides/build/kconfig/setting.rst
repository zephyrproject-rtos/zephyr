.. _setting_configuration_values:

Setting Kconfig configuration values
####################################

The :ref:`menuconfig and guiconfig interfaces <menuconfig>` can be used to test
out configurations during application development. This page explains how to
make settings permanent.

All Kconfig options can be searched in the Kconfig search page.

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

  .. code-block:: none

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

  Here's an example or an invisible symbol:

  .. code-block:: none

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

.. code-block:: none

   CONFIG_<symbol name>=<value>

There should be no spaces around the equals sign.

``bool`` symbols can be enabled or disabled by setting them to ``y`` or ``n``,
respectively. The ``FPU`` symbol from the example above could be enabled like
this:

.. code-block:: none

   CONFIG_FPU=y

.. note::

   A boolean symbol can also be set to ``n`` with a comment formatted like
   this:

   .. code-block:: none

      # CONFIG_SOME_OTHER_BOOL is not set

   This is the format you will see in the merged configuration in
   :file:`zephyr/.config`.

   This style is accepted for historical reasons: Kconfig configuration files
   can be parsed as makefiles (though Zephyr doesn't use this). Having
   ``n``-valued symbols correspond to unset variables simplifies tests in Make.

Other symbol types are assigned like this:

.. code-block:: none

   CONFIG_SOME_STRING="cool value"
   CONFIG_SOME_INT=123

Comments use a #:

.. code-block:: none

   # This is a comment

Assignments in configuration files are only respected if the dependencies for
the symbol are satisfied. A warning is printed otherwise. To figure out what
the dependencies of a symbol are, use one of the :ref:`interactive
configuration interfaces <menuconfig>` (you can jump directly to a symbol with
:kbd:`/`), or look up the symbol in the Kconfig search page.


.. _initial-conf:

The Initial Configuration
*************************

The initial configuration for an application comes from merging configuration
settings from three sources:

1. A ``BOARD``-specific configuration file stored in
   :file:`boards/<architecture>/<BOARD>/<BOARD>_defconfig`

2. Any CMake cache entries prefix with ``CONFIG_``

3. The application configuration

The application configuration can come from the sources below. By default,
:file:`prj.conf` is used.

1. If ``CONF_FILE`` is set, the configuration file(s) specified in it are
   merged and used as the application configuration. ``CONF_FILE`` can be set
   in various ways:

   1. In :file:`CMakeLists.txt`, before calling ``find_package(Zephyr)``

   2. By passing ``-DCONF_FILE=<conf file(s)>``, either directly or via ``west``

   3. From the CMake variable cache

2. Otherwise if ``CONF_FILE`` is set, and a single configuration file of the
   form :file:`prj_<build>.conf` is used, then if file
   :file:`boards/<BOARD>_<build>.conf` exists in same folder as file
   :file:`prj_<build>.conf`, the result of merging :file:`prj_<build>.conf` and
   :file:`boards/<BOARD>_<build>.conf` is used.

3. Otherwise, :file:`prj_<BOARD>.conf` is used if it exists in the application
   configuration directory.

4. Otherwise, if :file:`boards/<BOARD>.conf` exists in the application
   configuration directory, the result of merging it with :file:`prj.conf` is
   used.

5. Otherwise, if board revisions are used and
   :file:`boards/<BOARD>_<revision>.conf` exists in the application
   configuration directory, the result of merging it with :file:`prj.conf` and
   :file:`boards/<BOARD>.conf` is used.

6. Otherwise, :file:`prj.conf` is used if it exists in the application
   configuration directory

All configuration files will be taken from the application's configuration
directory except for files with an absolute path that are given with the
``CONF_FILE`` argument.

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


Configuring invisible Kconfig symbols
*************************************

When making changes to the default configuration for a board, you might have to
configure invisible symbols. This is done in
:file:`boards/<architecture>/<BOARD>/Kconfig.defconfig`, which is a regular
:file:`Kconfig` file.

.. note::

    Assignments in :file:`.config` files have no effect on invisible symbols,
    so this scheme is not just an organizational issue.

Assigning values in :file:`Kconfig.defconfig` relies on defining a Kconfig
symbol in multiple locations. As an example, say we want to set ``FOO_WIDTH``
below to 32:

.. code-block:: none

    config FOO_WIDTH
    	int

To do this, we extend the definition of ``FOO_WIDTH`` as follows, in
:file:`Kconfig.defconfig`:

.. code-block:: none

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

.. warning::

   When defining a symbol in multiple locations, dependencies are ORed together
   rather than ANDed together. It is not possible to make the dependencies of a
   symbol more restrictive by defining it in multiple locations.

   For example, the direct dependencies of the symbol below becomes
   ``DEP1 || DEP2``:

   .. code-block:: none

      config FOO
      	...
      	depends on DEP1

      config FOO
      	...
      	depends on DEP2

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

   .. code-block:: none

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

   .. code-block:: none

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
docstring (at the top of the file) goes over how symbol values are calculated
in detail.
