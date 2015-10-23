.. _apps_object_conf:

Microkernel Object Configuration
################################

Microkernel objects are explained fully in the :ref:`zephyr_primer`.
See :ref:`microkernel` for example MDEF entries.

Procedure
*********

.. _create_mdef:

Creating and Configuring an MDEF for a Microkernel Application
==============================================================

Create the MDEF to define microkernel objects used in your
application when they apply to the application as a whole.
You do not need to define every object before writing code. In
some cases, the necessary objects aren't obvious until you begin
writing code. However, all objects used in your code must be defined
before your application will compile successfully.

.. note::

  Nanokernel applications do not use an MDEF because microkernel
  objects cannot be used in applications of this type.

Before you begin
----------------

* Confirm your :file:`~/appDir` already exists.

* Confirm Zephyr environment variables are set for each console
  terminal; for instructions, see :ref:`apps_common_procedures`.

Steps
-----

1. Create an MDEF in your application directory
   (:file:`~/appDir ~) using
   the name you specified in your application Makefile.
   (See :ref:`Creating an Application Makefile`).

 .. code-block:: bash

  $ touch prj.mdef

 The default MDEF name is :file:`prj.mdef`.

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

.. note::

   Some microkernel objects, such as Task IRQs, are not
   defined in an :file:`.mdef` file.

The following example shows the content of the
:file:`samples/microkernel/apps/philosophers/proj.mdef`
for the Philosophers' sample application. The sample
uses seven tasks and six mutexes.

Example MDEF
------------

.. literalinclude:: ../../samples/microkernel/apps/philosophers/proj.mdef
   :linenos:

Related Topics
--------------

:ref:`src_makefiles_reqs`
:ref:`src_files_directories`
