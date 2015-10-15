.. _apps_kernel_conf:

Kernel Configuration
####################

The application's kernel is configured from a set of options that
can be customized for application-specific purposes. Each
configuration option is derived from the first source in which
it is specified:

   a. The value specified by the application’s current
      configuration; that is, its :file:`.config` file.

   b. The value specified by the application’s default
      configuration; that is, its :file:`prj.conf` file.

   c. The value specified by the platform configuration.

   d. The kernel’s default value for the configuration option.

.. note::

   When the default platform configuration settings are sufficient
   for your application, a :file:`prj.conf` file is not needed.
   Skip ahead to
  :ref:`Overriding the Application's Default Kernel Configuration`.


Procedures
**********

* `Defining the Application's Default Kernel Configuration`_

* `Overriding the Application's Default Kernel Configuration`_

.. _Defining the Application's Default Kernel Configuration:

The procedures that follow describe how to configure a :file:`prj.conf`
file and how to configure kernel options for microkernel and nanokernel
applications. For information on how to work with kernel option
inter-dependencies and platform configuration-default options, see the
:ref:`configuration`.

.. note::

   There are currently a number of experimental options not yet
   fully supported.

Defining the Application's Default Kernel Configuration
=======================================================

Create a :file:`prj.conf` file to define the application's
default kernel configuration. This file can contain
settings that override or augment platform-configuration settings.

The contents of the supported platform configuration files
can be viewed in :file:`~/rootDir/architectureDir/configs`.

Before you begin
----------------

* Confirm Zephyr environment variables are set for each console
  terminal using :ref:`apps_common_procedures`.

Steps
-----

1. Navigate to the :file:`appDir`, and create the :file:`prj.conf` file. Enter:

  .. code-block:: bash

   $ touch prj.conf


  The default name is :file:`prj.conf`. The filename must match
  the :option:`CONF_FILE =` entry in the application :file:`Makefile`.

2. Edit the file and add the appropriate configuration entries.

   a) Add each configuration entry on a new line.

   b) Begin each entry with :option:`CONFIG_`.

   c) Ensure that each entry contains no spaces
      (including on either side of the = sign).

   d) Use a # followed by a space to comment a line.

   This example shows a comment line and a platform
   configuration override in the :file:`prj.conf`.

  .. code-block:: c

   # Change the number of IRQs supported by the application
     CONFIG_NUM_IRQS=43

3. Save and close the file.


.. _Overriding the Application's Default Kernel Configuration:

Overriding the Application's Default Kernel Configuration
=========================================================

Override the application's default kernel configuration to
temporarily alter the application’s configuration, perhaps
to test the effect of a change.

.. _note::
   If you want to permanently alter the configuration you
   should revise the :file:`.conf` file.

Configure the kernel options using a menu-driven interface.
While you can add entries manually, using the configuration menu
is a preferred method.

Before you begin
----------------

* Review the kernel configuration options available and know
  which ones you want to temporarily set for your application.
  See the :ref:`configuration` for a brief description of each option.

* Be aware of any dependencies among the kernel configuration options.

* Confirm an application :file:`Makefile` exists for your application.

* Confirm the Zephyr environment variable is set for each console
  terminal; see :ref:`apps_common_procedures`.

Steps
-----

1.  Run the :command:`make menuconfig` rule to launch the
    menu-driven interface.

 a) In a terminal session, navigate to the application directory
    (:file:`~/appDir`).

 b) Enter the following command:

  .. code-block:: bash

   $ make menuconfig

  A question-based menu opens that allows you to set individual
  configuration options.

.. image:: figures/app_kernel_conf_1.png
    :width: 400px
    :align: center
    :height: 375px
    :alt: Main Configuration Menu

2.  Set kernel configuration values using the following
    key commands:

   * Use the arrow keys to navigate within any menu or list.

   * Press :kbd:`Enter` to select a menu item.

   * Type an upper case :kbd:`Y` or :kbd:`N` in the
     square brackets :guilabel:`[ ]` to
     enable or disable a kernel configuration option.

   * Type a numerical value in the round brackets :guilabel:`( )`.

   * Press :kbd:`Tab` to navigate the command menu at the
     bottom of the display.

   .. _note::

    When a non-default entry is selected for options that
    are nonnumerical, an asterisk :kbd:`*` appears between the
    square brackets in the display. There is nothing added added
    the display when you select the option's default.

3.  For information about any option, select the option and
    tab to :guilabel:`< Help >` and press :kbd:`Enter`.

    Press :kbd:`Enter` to return to the menu.

4.  After configuring the kernel options for your application,
    tab to :guilabel:`< Save >` and press :kbd:`Enter`.

    The following dialog opens with the :guilabel:`< Ok >`
    command selected:

.. image:: figures/app_kernel_conf_2.png
    :width: 400px
    :align: center
    :height: 100px
    :alt: Save Configuration Dialog


5.  Press :kbd:`Enter` to save the kernel configuration options
    to the default file name; alternatively, type a file
    name and press :kbd:`Enter`.

    Typically, you will save to the default file name unless
    you are experimenting with various configuration scenarios.

    An :file:`outdir` directory is created in the application
    directory. The outdir directory contains symbolic links
    to files under $(ZEPHYR_BASE).

   .. _note::

    At present, only a :file:`.config` file can be built. If
    you have saved files with different file names and want to build
    with one of these, change the file name to :file:`.config`.
    To keep your original :file:`.config`, rename it to something
    other than :file:`.config`.

    Kernel configuration files, such as the :file:`.config`
    file, are saved as hidden files in :file:`outdir`. To list
    all your kernel configuration files, enter :command:`ls -a`
    at the terminal prompt.

    The following dialog opens, displaying the file name the
    configuration was saved to.

.. image:: figures/app_kernel_conf_3.png
    :width: 400px
    :align: center
    :height: 150px
    :alt: Saved Configuration Name Dialog

6.  Press :kbd:`Enter` to return to the options menu.

7.  To load any saved kernel configuration file,
    tab to :guilabel:`< Load >` and press :kbd:`Enter`.

    The following dialog opens with the :guilabel:`< Ok >`
    command selected:

.. image:: figures/app_kernel_conf_4.png
    :width: 400px
    :align: center
    :height: 175px
    :alt: Configuration File Load Dialog

8.  To load the last saved kernel configuration file, press
    :guilabel:`< Ok >`, or to load another saved configuration
    file, type the file name, then select :guilabel:`< Ok >`.

9.  Press :kbd:`Enter` to load the file and return to the main
    menu.

10. To exit the menu configuration, tab to :guilabel:`< Exit >`
    and press :kbd:`Enter`.

    The following confirmation dialog opens with the
    :guilabel:`< Yes >` command selected.

.. image:: figures/app_kernel_conf_5.png
    :width: 400px
    :align: center
    :height: 100px
    :alt: Exit Dialog

11. Press :kbd:`Enter` to retire the menu display and
    return to the console command line.

**Next Steps**:
For microkernel applications, go to :ref:`Creating and
Configuring an MDEF File for a Microkernel Application`.

For nanokernel applications, go to :ref:`apps_code_dev`.