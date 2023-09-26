.. _mcumgr_handlers:

MCUmgr handlers
###############

Overview
********

MCUmgr functions by having group handlers which identify a group of functions relating to a
specific management area, which is addressed with a 16-bit identification value,
:c:enum:`mcumgr_group_t` contains the management groups available in Zephyr with their
corresponding group ID values. The group ID is included in SMP headers to identify which
group a command belongs to, there is also an 8-bit command ID which identifies the function of
that group to execute - see :ref:`mcumgr_smp_protocol_specification` for details on the SMP
protocol and header. There can only be one registered group per unique ID.

Implementation
**************

MCUmgr handlers can be added externally by application code or by module code, they do not have
to reside in the upstream Zephyr tree to be usable. The first step to creating a handler is to
create the folder structure for it, the typical Zephyr MCUmgr group layout is as follows:

.. code-block:: none

   <dir>/grp/<grp_name>_mgmt/
   ├── CMakeLists.txt
   ├── Kconfig
   ├── include
   ├──── <grp_name>_mgmt.h
   ├──── <grp_name>_mgmt_callbacks.h
   ├── src
   └──── <grp_name>_mgmt.c

Note that the header files in upstream Zephyr MCUmgr handlers reside in the
``zephyr/include/zephyr/mgmt/mcumgr/grp/<grp_name>_mgmt`` directory to allow the files to be
globally included by applications.

Initial header <grp_name>_mgmt.h
================================

The purpose of the header file is to provide defines which can be used by the MCUmgr handler
itself and application code, e.g. to reference the command IDs for executing functions. An example
file would look similar to:

.. literalinclude:: ../../../tests/subsys/mgmt/mcumgr/handler_demo/example_as_module/include/example_mgmt.h
   :language: c
   :linenos:

This provides the defines for 2 command ``test`` and ``other`` and sets up the SMP version 2 error
responses (which have unique error codes per group as opposed to the legacy SMP version 1 error
responses that return a :c:enum:`mcumgr_err_t` - there should always be an OK error code with the
value 0 and an unknown error code with the value 1. The above example then adds an error code of
``not wanted`` with value 2. In addition, the group ID is set to be
:c:enum:`MGMT_GROUP_ID_PERUSER`, which is the start group ID for user-defined groups, note that
group IDs need to be unique so other custom groups should use different values, a central index
header file (as upstream Zephyr has) can be used to distribute group IDs more easily.

Initial header <grp_name>_mgmt_callbacks.h
==========================================

The purpose of the header file is to provide defines which can be used by the MCUmgr handler
itself and application code, e.g. to reference the command IDs for executing functions. An example
file would look similar to:

.. literalinclude:: ../../../tests/subsys/mgmt/mcumgr/handler_demo/example_as_module/include/example_mgmt_callbacks.h
   :language: c
   :linenos:

This sets up a single event which application (or module) code can register for to receive a
callback when the function handler is executed, which allows the flow of the handler to be
changed (i.e. to return an error instead of continuing). The event group ID is set to
:c:enum:`MGMT_EVT_GRP_USER_CUSTOM_START`, which is the start event ID for user-defined groups,
note that event IDs need to be unique so other custom groups should use different values, a
central index header file (as upstream Zephyr has) can be used to distribute event IDs more
easily.

Initial source <grp_name>_mgmt.c
================================

The purpose of this source file is to handle the incoming MCUmgr commands, provide responses, and
register the transport with MCUmgr so that commands will be sent to it. An example file would
look similar to:

.. literalinclude:: ../../../tests/subsys/mgmt/mcumgr/handler_demo/example_as_module/src/example_mgmt.c
   :language: c
   :linenos:

The above code creates 2 function handlers, ``test`` which supports read requests and takes 2
required parameters, and ``other`` which supports write requests and takes 1 optional parameter,
this function handler has an optional notification callback feature that allows other parts of
the code to listen for the event and take any required actions that are necessary or prevent
further execution of the function by returning an error, further details on MCUmgr callback
functionality can be found on :ref:`mcumgr_callbacks`.

Note that other code referencing callbacks for custom MCUmgr handlers needs to include both the
base Zephyr callback include file and the custom handler callback file, only in-tree Zephyr
handler headers are included when including the upstream Zephyr callback header file.

Initial Kconfig
===============

The purpose of the Kconfig file is to provide options which users can enable or change relating
to the functionality of the handler being implemented. An example file would look similar to:

.. literalinclude:: ../../../tests/subsys/mgmt/mcumgr/handler_demo/Kconfig
   :language: kconfig

Initial CMakeLists.txt
======================

The CMakeLists.txt file is used by the build system to setup files to compile, include
directories to add and specify options that can be changed. A basic file only need to include the
source files if the Kconfig options are enabled. An example file would look similar to:

.. tabs::

   .. group-tab:: Zephyr module

      .. literalinclude:: ../../../tests/subsys/mgmt/mcumgr/handler_demo/example_as_module/CMakeLists.txt
         :language: cmake

   .. group-tab:: Application

      .. literalinclude:: ../../../tests/subsys/mgmt/mcumgr/handler_demo/CMakeLists.txt
         :language: cmake
         :start-after: Include handler files

Including from application
**************************

Application-specific MCUmgr handlers can be added by creating/editing application build files.
Example modifications are shown below.

Example CMakeLists.txt
======================

The application ``CMakeLists.txt`` file can load the CMake file for the example MCUmgr handler by
adding the following:

.. code-block:: cmake

    add_subdirectory(mcumgr/grp/<grp_name>)

Example Kconfig
===============

The application Kconfig file can include the Kconfig file for the example MCUmgr handler by adding
the following to the ``Kconfig`` file in the application directory (or creating it if it does not
exist):

.. code-block:: kconfig

    rsource "mcumgr/grp/<grp_name>/Kconfig"

    # Include Zephyr's Kconfig
    source "Kconfig.zephyr"

Including from Zephyr Module
****************************

Zephyr :ref:`modules` can be used to add custom MCUmgr handlers to multiple different applications
without needing to duplicate the code in each application's source tree, see :ref:`module-yml` for
details on how to set up the module files. Example files are shown below.

Example zephyr/module.yml
=========================

This is an example file which can be used to load the Kconfig and CMake files from the root of the
module directory, and would be placed at ``zephyr/module.yml``:

.. code-block:: yaml

    build:
      kconfig: Kconfig
      cmake: .

Example CMakeLists.txt
======================

This is an example CMakeLists.txt file which loads the CMake file for the example MCUmgr handler,
and would be placed at ``CMakeLists.txt``:

.. code-block:: cmake

    add_subdirectory(mcumgr/grp/<grp_name>)

Example Kconfig
===============

This is an example Kconfig file which loads the Kconfig file for the example MCUmgr handler, and
would be placed at ``Kconfig``:

.. code-block:: kconfig

    rsource "mcumgr/grp/<grp_name>/Kconfig"

Demonstration handler
*********************

There is a demonstration project which includes configuration for both application and zephyr
module-MCUmgr handlers which can be used as a basis for created your own in
:zephyr_file:`tests/subsys/mgmt/mcumgr/handler_demo/`.
