.. _apps_build:

Build an Application
####################

The Zephyr build system compiles and links all components of an application
into a single application image that can be run on simulated hardware or real
hardware.

.. contents:: Procedures
   :local:
   :depth: 1

Building an Application
=======================

The build system allows you to easily build an application using the
application's existing configuration. However, you can also build it
using different configuration settings, if desired.

Before you begin
----------------

* Ensure you have added all application-specific code to the application
  directory, as described in :ref:`apps_code_dev`. Ensure each source code
  directory and sub-directory has its own :file:`Makefile`.

* Ensure you have configured the application's kernel, as described
  in :ref:`apps_kernel_conf`.

* Ensure the Zephyr environment variables are set for each console terminal;
  see :ref:`apps_common_procedures`.

Steps
-----

#. Navigate to the application directory :file:`~/appDir`.

#. Enter the following command to build the application's :file:`zephyr.elf`
   image using the configuration settings for the board type specified
   in the application's :file:`Makefile`.

   .. code-block:: console

       $ make

   If desired, you can build the application using the configuration settings
   specified in an alternate :file:`.conf` file using the :code:`CONF_FILE`
   parameter. These settings will override the settings in the application's
   :file:`.config` file or its default :file:`.conf` file. For example:

   .. code-block:: console

       $ make CONF_FILE=prj.alternate.conf

   If desired, you can build the application for a different board type
   than the one specified in the application's :file:`Makefile`
   using the :code:`BOARD` parameter. For example:

   .. code-block:: console

       $ make BOARD=arduino_101

   Both the :code:`CONF_FILE` and :code:`BOARD` parameters can be specified
   when building the application.

Rebuilding an Application
=========================

Application development is usually fastest when changes are continually tested.
Frequently rebuilding your application makes debugging less painful
as the application becomes more complex. It's usually a good idea to
rebuild and test after any major changes to the application's source files,
Makefiles, or configuration settings.

.. important::

    The Zephyr build system rebuilds only the parts of the application image
    potentially affected by the changes. Consequently, rebuilding an application
    is often significantly faster than building it the first time.

Steps
-----

#. Follow the steps specified in `Building an Application`_ above.

Recovering from a Build Failure
===============================

Sometimes the build system doesn't rebuild the application correctly
because it fails to recompile one or more necessary files. You can force
the build system to rebuild the entire application from scratch with the
following procedure:

Steps
-----

#. Navigate to the application directory :file:`~/appDir`.

#. Enter the following command to delete the application's generated files
   for the specified board type, except for the :file:`.config` file that
   contains the application's current configuration information.

   .. code-block:: console

       $ make [BOARD=<type>] clean

   Alternatively, enter the following command to delete *all* generated files
   for *all* board types, including the :file:`.config` files that contain
   the application's current configuration information for those board types.

   .. code-block:: console

       $ make pristine

#. Rebuild the application normally following the steps specified
   in `Building an Application`_ above.
