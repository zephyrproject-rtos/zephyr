.. _microkernel_apps:

How to Develop Microkernel Applications
#######################################

Microkernel applications are comprised of .c and . h files that are
integrated with the |codename| using MDEF files, Makefiles and .conf files.
The following procedures explain how to accomplish this using examples.

Before Starting the Development of a Microkernel App
****************************************************

Before you begin, read the :ref:`technical_overview` and the :ref:`
quick_start`. Then read the :ref:`collaboration_guidelines`. Finally, review
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
********************

The :abbr:`MDEF (Microkernel DEfinitions File)` contains the configuration
entries for the kernel's objects. Therefore, it must include all microkernel
objects used in the C code of the application. Each line in a MDEF file
defines one item, unless it is empty or a comment line. All lines starting
with a ``%`` are comments.

The contents of an MDEF file can be, for example:

.. code-block:: py
   :lineos:

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

   % RESOURCE NAME
   % =====================
     RESOURCE PRINTRES
     RESOURCE GRAPHRES
     RESOURCE POPPARAMRES
     RESOURCE PUSHPARAMRES

Each object must follow a specific syntax. The name of an object must be an
alphanumeric, with an alphabetical first character. Upper- and lowercase
letters are allowed, but embedded spaces are not. The convention is to give
each object a name that is in all uppercase letters. This makes it easy to
distinguish between object names and variable names. Finally, all names must
be unique. A pipe cannot have the same name as a FIFO, even though they are
different types.

See :ref:`mdef_parameters` for the syntax of the most important microkernel
objects.

