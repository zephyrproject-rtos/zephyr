.. _cleanup_api:

Cleanup Classes
###############

.. contents::
    :local:
    :depth: 2

Overview
********

The Cleanup Classes API provides a mechanism for automatic resource cleanup when
variables go out of scope. This is similar to RAII in C++ or defer statements
in Go. By leveraging compiler support for the ``__cleanup`` attribute,
this API helps prevent resource leaks and simplifies error handling by ensuring
cleanup code is executed automatically.

This feature is particularly useful for:

* Automatic unlocking of mutexes and semaphores
* Automatic freeing of dynamically allocated memory
* Ensuring cleanup actions occur on all code paths (including early returns)
* Reducing boilerplate cleanup code

.. warning::

   The cleanup mechanism is implemented using the ``__cleanup`` attribute,
   if the toolchain doesn't support this attribute, the macros expand to empty
   definitions and the API is not available.

   For this reason this API is intended solely for user applications and not for
   the kernel itself or other subsystems.

Core Concepts
*************

The cleanup API provides three main abstractions:

Cleanup Classes
===============

A cleanup class defines a type with automatic constructor and destructor
behavior. Variables declared with a cleanup class are initialized using a
constructor function and automatically cleaned up using a destructor function
when they go out of scope.

Guards
======

Guards are specialized cleanup classes that automatically acquire a lock or
resource on initialization and release it when going out of scope. This pattern
is commonly used with mutexes, semaphores, and other synchronization primitives.

Defers
======

Defers are cleanup classes that execute a specified function when the variable
goes out of scope, without acquiring anything on initialization. This is similar
to the ``defer`` statement in languages like Go.

Defining Cleanup Classes
************************

Custom Cleanup Classes
======================

Use :c:macro:`DEFINE_CLASS` to define a custom cleanup class with constructor
and destructor functions:

.. code-block:: c

    static inline struct flash_area *flash_area_constructor(int area_id)
    {
        struct flash_area *fa;

        if (flash_area_open(area_id, &fa) < 0) {
            return NULL;
        }

        return fa;
    }

    static inline void flash_area_destructor(struct flash_area *fa)
    {
        if (fa != NULL) {
            flash_area_close(fa);
        }
    }

    // Define the cleanup class
    DEFINE_CLASS(flash_area, struct flash_area *, flash_area_destructor(_T),
                 flash_area_constructor(area_id), int area_id)

    static int some_function(void)
    {
        // Declare 'fa' with automatic cleanup
        CLASS(flash_area, fa)(FIXED_PARTITION_ID(storage_partition));
        if (fa == NULL) {
            return -EINVAL;  // Destructor is still called
        }

        // Use fa normally
        printk("Has driver: %d\n", flash_area_has_driver(fa));

        // No need to manually close - destructor is called automatically
        return 0;
    }

The ``_T`` variable in the destructor expression contains the value of the
variable being cleaned up.

Guard Classes
=============

Use :c:macro:`DEFINE_GUARD` to define a guard that acquires a lock on
initialization and releases it on scope exit:

.. code-block:: c

    // Example guard definition (already provided by kernel.h)
    DEFINE_GUARD(k_mutex, struct k_mutex *,
                 (void)k_mutex_lock(_T, K_FOREVER),
                 (void)k_mutex_unlock(_T))

    static K_MUTEX_DEFINE(lock);

    void critical_section(void)
    {
        GUARD(k_mutex)(&lock);

        // Lock is held here
        // Perform critical operations

        // Lock is automatically released when guard goes out of scope
    }

Defer Classes
=============

Use :c:macro:`DEFINE_DEFER` to define a defer that executes a cleanup function:

.. code-block:: c

    // Define a defer for a custom cleanup function
    static void cleanup_resources(void)
    {
        // Cleanup code here
    }

    DEFINE_DEFER(cleanup_resources)

    void some_function(void)
    {
        DEFER(cleanup_resources)();

        // Do work...

        // cleanup_resources() is called automatically
    }

For functions with parameters:

.. code-block:: c

    // Example deferred k_free (already provided by kernel.h)
    DEFINE_DEFER(k_free, void *)

    void allocate_and_use(void)
    {
        void *ptr = k_malloc(100);
        DEFER(k_free)(ptr);

        // Use ptr...

        // k_free(ptr) is called automatically
    }

Usage Notes
***********

Order of Cleanup
================

Cleanup functions are called in reverse order of declaration (LIFO - Last In,
First Out), which matches the natural nesting of resources:

.. code-block:: c

    {
        GUARD(k_mutex)(&lock);           // Acquired first
        void *ptr = k_malloc(100);
        DEFER(k_free)(ptr);               // Registered second

        // Do work...

    }  // ptr is freed first, then mutex is unlocked

Scope Rules
===========

Cleanup occurs when the variable goes out of scope, which includes:

* Reaching the end of a block
* Early return statements
* Break or continue in loops
* Goto statements that jump out of scope

.. code-block:: c

    void example_with_early_exit(struct k_mutex *lock)
    {
        GUARD(k_mutex)(lock);

        if (error_condition) {
            return;  // Guard cleanup happens here
        }

        // Normal path

    }  // Guard cleanup also happens here

API Reference
*************

.. doxygengroup:: cleanup_interface
