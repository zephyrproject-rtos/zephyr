.. _microkernel_apps:

How to Develop Microkernel Applications
#######################################

Microkernel applications are comprised of .c and . h files that are
integrated with the |codename| using MDEF files, Makefiles and .conf files.

Developing a microkernel application can be accomplished in five steps:

#. Plan your application and take note of all microkernel objects it will
   need.

#. Create the MDEF file for those microkernel objects.

#. Code the application source files.

#. Create the Makefiles to add the application's source files to the
   kernel's build system.

#. Compile and test your application.

The following procedures explain how to accomplish this using examples.

Before Starting the Development of a Microkernel App
****************************************************

Before you begin, read the :ref:`overview` and the :ref:`quick_start`.
Then read the :ref:`collaboration_guidelines`. Finally, review
the developers information contained in :ref:`developing_zephyr` and build a
sample application, like :file:`hello.c`.

Once you have completed those tasks, you should be able to answer:

* How does the |codename| and its key objects work?

* What is the difference between a nanokernel and a microkernel?

* How is the |codename| built?

* What rules and conventions must the development follow?

If you are able to answer this questions, you can start developing your
application.

Creating an MDEF File
*********************

The :abbr:`MDEF (Microkernel DEfinitions File)` contains the configuration
entries for the kernel's objects. Therefore, it must include all microkernel
objects used in the C code of the application. Each line in a MDEF file
defines one item, unless it is empty or a comment line. All lines starting
with a ``%`` are comments.

The contents of an MDEF file can be, for example:

.. code-block:: console
   :linenos:

   % TASKGROUP    NAME
   % =======================
     TASKGROUP    ITERATIONS

   % TASK NAME         PRIO ENTRY           STACK  GROUPS
   % ========================================================
     TASK MASTER       7 master             1024   [EXE]
     TASK PLOTTER      5 plotter            1024   [EXE]
     TASK TASK10       6 task               2048   [ITERATIONS]
     TASK TASK11       6 task               2048   [ITERATIONS]
     TASK TASK12       6 task               2048   [ITERATIONS]
     TASK TASK13       6 task               2048   [ITERATIONS]

   % FIFO NAME         DEPTH WIDTH
   % =============================
     FIFO PLOTQ        200    20
     FIFO COORDQ        16    16
     FIFO SCREENQ       16    20

   % SEMA NAME
   % =============
     SEMA FINISHED

   % MUTEX NAME
   % =====================
     MUTEX PRINT
     MUTEX GRAPH
     MUTEX POPPARAM
     MUTEX PUSHPARAM

Each object must follow a specific syntax. The name of an object must be an
alphanumeric, with an alphabetical first character. Upper- and lowercase
letters are allowed, but embedded spaces are not. The convention is to give
each object a name that is in all uppercase letters. This makes it easy to
distinguish between object names and variable names. Finally, all names must
be unique. A pipe cannot have the same name as a FIFO, even though they are
different types.

See :ref:`microkernel` for the correct syntax for the most important
microkernel objects.

Coding the Application's Source Files
*************************************

We recommend you follow the project's :ref:`coding_style` when coding your
application. The required MDEF file can be in any folder of your
application as long as the path is referenced in the application's Makefile.

The application's root folder must contain a Makefile that links the
application to the kernel, see :ref:`root_makefile` for details. Each folder
in your source tree needs to have a Makefile that links the folder's
contents with the rest of your source tree, see :ref:`source_makefile` for
details.

Finally, remember that your application will be compiled into the
kernel. If all MDEF files and Makefiles are correct and in place, the build
system links your application with the kernel. The kernel delivers binaries,
one for each selected platform, for testing.

.. _app_makefile:

Creating the Makefiles for the Application
******************************************

This section explains how the Makefiles within the folders of your
application link it to the kernel. The build system's will assume that there
is an application's root folder with a :file:`src` folder containing all
source files. The name of the folder can be changed to whatever name suits
your needs. The :file:`src` folder can have as many subfolders as needed but
all folders must contain a Makefile.

The contents of the Makefiles can fit the needs of your application.
However, some contents are required for a successful integration. The
contents required in the application's Makefile at the main folder differ
from those required in the Makefiles inside the source folders.

For the detailed information regarding the build system's Makefiles see:
:ref:`kbuild_makefiles`.

.. _root_makefile:

The Application's Root Folder Makefile
======================================

The Makefile in the application's root folder must contain at least these
entries:

* :makevar:`MDEF_FILE`: The name of the the application's MDEF file.

* :makevar:`KERNEL_TYPE`: Either `nano` for nanokernel applications or
   `micro` for microkernel applications.

* :makevar:`PLATFORM_CONFIG`: The name of the platform configuration that
   will be used as a default.

* :makevar:`CONF_FILE`: The name of the .conf file or files of the
   application.

* ``include ${ZEPHYR_BASE}/Makefile.inc`` This instruction adds the contents
   of the :file:`Makefile.inc` found in the kernel's root folder.

For all the information regarding the Makefile variables see:
:ref:`kbuild_project`.

Examples
--------

The following example shows a generic application's root folder Makefile:

.. code-block:: make
   :linenos:
   :emphasize-lines: 4, 6

   MDEF_FILE = application.mdef
   KERNEL_TYPE = micro
   PLATFORM_CONFIG ?= basic_atom
   CONF_FILE = application_$(ARCH).conf

   include ${ZEPHYR_BASE}/Makefile.inc

Line 4 shows how to conditionally add files. The build system replaces the
variable ``$(ARCH)`` for each supported architecture.

The entry in line 6 includes all the entries located in :file:`Makefile.inc`
at the kernel's root folder. The entries let the build system know, that
your application has to be included as part of the build.

The next example comes from the kernel's code, specifically from
:file:`samples/microkernel/apps/hello_world/Makefile`:

.. literalinclude:: ../../samples/microkernel/apps/hello_world/Makefile
   :language: make
   :lines: 1-6
   :emphasize-lines: 1, 4
   :linenos:

The file :file:`proj.mdef` from line 1 can be found in
:file:`microkernel/apps/hello_world/` and contains the configuration entries
of all microkernel objects used in the application. For more information
regarding MDEF files see, the :ref:`microkernel` documentation.

The entry in line 4 references the files :file:`proj_arm.conf` and
:file:`proj_x86.conf` also found at that location. Those files include the
configuration values that differ from the default. For more information
regarding these configuration snippets see: :ref:`configuration_snippets`.


The Application's Source Folders Makefiles
==========================================

The Makefiles in the source folders add the files within the folders to the
build system. All information about adding source files and directories to
your project can be found in :ref:`makefile_conventions`.

Examples
--------

Example 1 shows the source folder Makefile of the microkernel Hello World
application.

Example 1:

.. literalinclude:: ../../samples/microkernel/apps/hello_world/src/Makefile
   :language: make
   :lines: 1-4
   :emphasize-lines: 1, 3
   :linenos:

Line 1 allows the application's source file access to the functions included
in the project's .h files. Line 3 adds the :file:`hello.c` to the build
system.

Example 2 shows the source folder Makefile of the microkernel Philosophers
application.

Example 2:

.. literalinclude:: ../../samples/microkernel/apps/philosophers/src/Makefile
   :language: make
   :lines: 1-4
   :emphasize-lines: 3
   :linenos:

Line 3 adds the :file:`phil_fiber.c` and :file:`phil_task.c` files. Multiple
files can be added on a single line by separating them with a space.
