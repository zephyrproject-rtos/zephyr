.. _usermode_sharedmem:

Application Shared Memory
#########################

.. note::

   In this document, we will cover the basic usage of enabling shared
   memory using a template around app_memory subsystem.

Overview
********

The use of subsystem app_memory in userspace allows control of
shared memory between threads.  The foundation of the implementation
consists of memory domains and partitions. Memory partitions are created
and used in the definition of variable to group them into a
common space.  The memory partitions are linked to domains
that are then assigned to a thread.  The process allows selective
access to memory from a thread and sharing of memory between two
threads by assigning a partition to two different domains.  By using
the shared memory template, code to protect memory can be used
on different platform without the application needing to implement
specific handlers for each platform.  Note the developer should understand
the hardware limitations in context to the maximum number of memory
partitions available to a thread.  Specifically processors with MPU's
cannot support the same number of partitions as a MMU.

This specific implementation adds a wrapper to simplify the programmers
task of using the app_memmory subsystem through the use of macros and
a python script to generate the linker script.  The linker script provides
the proper alignment for processors requiring power of two boundaries.
Without the wrapper, a developer is required to implement custom
linker scripts for each processor the project.

The general usage is as follows. Define CONFIG_APP_SHARED_MEM=y in the
proj.conf file in the project folder.  Include app_memory/app_memdomain.h
in the userspace source file.  Mark the variable to be placed in
a memory partition.  The two markers are for data and bss respectively:
_app_dmem(id) and _app_bmem(id).  The id is used as the partition name.
The resulting section name can be seen in the linker.map as
"data_smem_id" and "data_smem_idb".

To create a k_mem_partition, call the macro app_mem_partition(part0)
where "part0" is the name then used to refer to that partition.
This macro only creates a function and necessary data structures for
the later "initialization".

To create a memory domain for the partition, the macro app_mem_domain(dom0)
is called where "dom0" is the name then used for the memory domain.
To initialize the partition (effectively adding the partition
to a linked list), init_part_part0() is called. This is followed
by init_app_memory(), which walks all partitions in the linked
list and calculates the sizes for each partition.

Once the partition is initialized, the domain can be
initialized with init_domain_dom0(part0) which initializes the
domain with partition part0.

After the domain has been initialized, the current thread
can be added using add_thread_dom0(k_current_get()).

Example:

.. code-block:: c

            /* create partition at top of file outside functions */
            app_mem_partition(part0);
            /* create domain */
            app_mem_domain(dom0);
            /* assign variables to the domain */
            _app_dmem(dom0) int var1;
            _app_bmem(dom0) static volatile int var2;

            int main()
            {
                    init_part_part0();
                    init_app_memory();
                    init_domain_dom0(part0);
                    add_thread_dom0(k_current_get());
                    ...
            }

If multiple partitions are being created, a variadic
preprocessor macro can be used as provided in
app_macro_support.h:

.. code-block:: c

 FOR_EACH(app_mem_partition, part0, part1, part2);

or, for multiple domains, similarly:

.. code-block:: c

 FOR_EACH(app_mem_domain, dom0, dom1);

Similarly, the init_part_* can also be used in the macro:

.. code-block:: c

 FOR_EACH(init_part, part0, part1, part2);


