.. _apps_build:

Build an Application
####################

The build process unifies all components of the application into
a coherent application image that can be run on both simulated and real
hardware targets.

.. _building_base:

Building a Base Application
===========================

Build a base application image to test functionality in a simulated
environment and to ultimately run on your hardware target. Before
building, keep in mind the following:

* Each source code directory and sub-directory needs a directory-specific
  :file:`Makefile`.

* The :envvar:`$(ZEPHYR_BASE)` environment variable must be set for each
  console terminal as outlined in :ref:`apps_common_procedures`.

To build the image, navigate to the :file:`~/appDir`. From here you can
build an image for a single target with :command:`make`.


.. _developing_app:

Developing the Application
==========================

The app development process works best when changes are continually tested.
Frequently rebuilding with :command:`make` makes debugging less painful
as your application becomes more complex. It's usually a good idea to
rebuild and test after any major changes to source files, Makefiles,
.conf, or .mdef.

.. important::

   The Zephyr build system rebuilds only the parts of the application image
   potentially affected by the changes; as such, the application may rebuild
   significantly faster than it did when it was first built.


Recovering from Build Failure
-----------------------------

Sometimes the build system doesn't rebuild the application correctly
because it fails to recompile one or more necessary files. You can force
the build system to rebuild the entire application from scratch with the
following procedure:

#. Navigate to the application directory :file:`~/appDir`.

#. Run :command:`$ make clean`, or manually delete the generated files,
   including the :file:`.config` file.

#. Run :command:`$ make pristine`.

#. You have the option to configure your project by running
   :command:`$ make menuconfig`. If you choose not to configure your project
   via :command:`menuconfig`, you can choose a configuration tailored for a
   supported board later.
   If you choose to use :command:`$ make menuconfig` be prepared to configure
   all the parameters correctly for your specific board.

#. Rebuild the application normally. Run :command:`$ make`. You can choose to
   specify a default configuration for a supported board using the parameter
   :command:`BOARD`. For example: :command:`$ make BOARD=arduino_101`.
   You can see the boards that currently support a default configuration by
   running the command :command:`$ make help`

#. Optionally, you can override the :file:`.config` file configuration (obtained
   as a result of :command:`menuconfig` or :command:`BOARD` parameters) by using
   the application’s :file:`.conf` file. Declare the kernel configuration settings
   that cover the specific needs of your project.


.. note::
   We recommend to use the :command:`BOARD` parameter, since it will load
   a preset configuration already tested to work properly with
   that board. You can always tune the board configuration. Override specific
   configuration elements by providing a configuration snippet file. Let the build
   system know about it with the :command:`CONF_FILE` environment variable.
