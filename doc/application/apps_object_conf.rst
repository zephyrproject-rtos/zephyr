.. _apps_object_conf:

Microkernel Object Configuration
################################

Microkernel objects are explained fully in the :ref:`zephyr_primer`.
See :ref:`microkernel` for example MDEF entries.

Procedure
*********

.. _Creating and Configuring an MDEF File for a Microkernel Application:

Creating and Configuring an MDEF File for a Microkernel Application
===================================================================

Create the MDEF file to define microkernel objects used in your
application when they apply to the application as a whole.
You do not need to define every object before writing code. In
some cases, the necessary objects aren't obvious until you begin
writing code. However, all objects used in your code must be defined
before your application will compile successfully.

.. _note::

  Nanokernel applications do not use an MDEF file because microkernel
  objects cannot be used in applications of this type.

Before you begin
----------------

* Confirm your :file:`~/appDir` already exists.

* Confirm Zephyr environment variables are set for each console
  terminal; for instructions, see :ref:`apps_common_procedures`.

Steps
-----

1. Create an MDEF file in your application directory
   (:file:`~/appDir ~) using
   the name you specified in your application Makefile.
   (See :ref:`Creating an Application Makefile`).

 .. code-block:: bash

  $ touch prj.mdef

 The default MDEF file name is :file:`prj.mdef`.

2. Open the file using a standard text editor.

3. Add settings to the file to suit your application.

 The syntax for objects that can be defined in :file:`.mdef`
 is:

 TASK name priority entry_point stack_size groups

 TASKGROUP name

 MUTEX name

 SEMA name

 FIFO name depth width

 PIPE name buffer_size

 MAILBOX name

 MAP name num_blocks block_size

 POOL name min_block_size max_block_size numMax

.. _note::

  Some microkernel objects, such as Task IRQs, are not
  defined in an :file:`.mdef` file.

The following example shows the content of the :file:`.mdef`
file for the Philosophers' sample application. The sample
uses seven tasks and six mutexes.

Example MDEF File
-----------------

.. code-block:: c

 % Application : DiningPhilosophers
 % TASKGROUP NAME
 % ==============
 TASKGROUP PHI
 % TASK NAME PRIO ENTRY STACK GROUPS
 % ==================================================
 TASK philTask 5 philDemo 1024 [EXE]
 TASK phi1Task0 6 philEntry 1024 [PHI]
 TASK philTask1 6 philEntry 1024 [PHI]
 TASK philTask2 6 philEntry 1024 [PHI]
 TASK philTask3 6 philEntry 1024 [PHI]
 TASK philTask4 6 philEntry 1024 [PHI]
 TASK philTask5 6 philEntry 1024 [PHI]
 % MUTEX NAME
 % ================
 MUTEX forkMutex0
 MUTEX forkMutex1
 MUTEX forkMutex2
 MUTEX forkMutex3
 MUTEX forkMutex4
 MUTEX forkMutex5


Related Topics
--------------

:ref:`Understanding src Directory Makefile Requirements`
:ref:`Adding Source Files and Makefiles to src Directories`
