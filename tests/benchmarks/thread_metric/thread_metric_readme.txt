              Thread-Metric RTOS Test Suite


1. Thread-Metric Test Suite

The Thread-Metric test suite consists of 8 distinct RTOS
tests that are designed to highlight commonly used aspects
of an RTOS. The test measures the total number of RTOS events
that can be processed during a specific timer interval. A 30
second time interval is recommended.

1.1. Basic Processing Test

This is the baseline test consisting of a single thread. This
should execute the same on every operating system. Test values
from testing with different RTOS products should be scaled
relative to the difference between the values of this test.

1.2. Cooperative Scheduling Test

This test consists of 5 threads created at the same priority that
voluntarily release control to each other in a round-robin fashion.
Each thread will increment its run counter and then relinquish to
the next thread.  At the end of the test the counters will be verified
to make sure they are valid (should all be within 1 of the same
value). If valid, the numbers will be summed and presented as the
result of the cooperative scheduling test.

1.3. Preemptive Scheduling Test

This test consists of 5 threads that each have a unique priority.
In this test, all threads except the lowest priority thread are
left in a suspended state. The lowest priority thread will resume
the next highest priority thread.  That thread will resume the
next highest priority thread and so on until the highest priority
thread executes. Each thread will increment its run count and then
call thread suspend.  Eventually the processing will return to the
lowest priority thread, which is still in the middle of the thread
resume call. Once processing returns to the lowest priority thread,
it will increment its run counter and once again resume the next
highest priority thread - starting the whole process over once again.

1.4. Interrupt Processing Test

This test consists of a single thread. The thread will cause an
interrupt (typically implemented as a trap), which will result in
a call to the interrupt handler. The interrupt handler will
increment a counter and then post to a semaphore. After the
interrupt handler completes, processing returns to the test
thread that initiated the interrupt. The thread then retrieves
the semaphore set by the interrupt handler, increments a counter
and then generates another interrupt.

1.5. Interrupt Preemption Processing Test

This test is similar to the previous interrupt test. The big
difference is the interrupt handler in this test resumes a
higher priority thread, which causes thread preemption.

1.6. Message Processing Test

This test consists of a thread sending a 16 byte message to a
queue and retrieving the same 16 byte message from the queue.
After the send/receive sequence is complete, the thread will
increment its run counter.

1.7. Synchronization Processing Test

This test consists of a thread getting a semaphore and then
immediately releasing it. After the get/put cycle completes,
the thread will increment its run counter.

1.8. RTOS Memory allocation

This test consists of a thread allocating a 128-byte block and
releasing the same block. After the block is released, the thread
will increment its run counter.

2. Zephyr Modifications

A few minor modifications have been made to the Thread-Metric source
code to resolve some minor issues found during porting.

2.1. tm_main() -> main()

Zephyr's version of this benchmark has modified the original tm_main()
to become main().

2.2. Thread entry points

Thread entry points used by Zephyr have a different function signature
than that used by the original Thread-Metric code. These functions
have been updated to match Zephyr's.

2.3. Reporting thread

Zephyr's version does not spawn a reporting thread. Instead it calls
the reporting function directly. This helps ensure that the test
operates correctly on QEMU platorms.

2.4. Directory structure

Each test has been converted to its own project. This has necessitated
some minor changes to the directory structure as compared to the
original version of this benchmark.

The source code to the Thread-Metric test suite is organized into
the following files:

         File                                           Meaning

tm_basic_processing_test.c                    Basic test for determining board
                                                 processing capabilities
tm_cooperative_scheduling_test.c              Cooperative scheduling test
tm_preemptive_scheduling_test.c               Preemptive scheduling test
tm_interrupt_processing_test.c                No-preemption interrupt processing
                                                 test
tm_interrupt_preemption_processing_test.c     Interrupt preemption processing
                                                 test
tm_message_processing_test.c                  Message exchange processing test
tm_synchronization_processing_test.c          Semaphore get/put processing test
tm_memory_allocation_test.c                   Basic memory allocation test
tm_porting_layer_zephyr.c                     Specific porting layer source
                                                 code for Zephyr

3 Porting

3.1 Porting Layer

As for the porting layer defined in tm_porting_layer_template.c, this file contain
shell services of the generic RTOS services used by the actual tests. The
shell services provide the mapping between the tests and the underlying RTOS.
The following generic API's are required to map any RTOS to the performance
measurement tests:


    void  tm_initialize(void (*test_initialization_function)(void));

    This function is typically called by the application from its
    main() function. It is responsible for providing all the RTOS
    initialization, calling the test initialization function as
    specified, and then starting the RTOS.

    int  tm_thread_create(int thread_id, int priority, void (*entry_function)(void));

    This function creates a thread of the specified priority where 1 is
    the highest and 16 is the lowest.  If successful, TM_SUCCESS
    returned. If an error occurs, TM_ERROR is returned. The created thread
    is not started.

    int  tm_thread_resume(int thread_id);

    This function resumes the previously created thread specified by
    thread_id. If successful, a TM_SUCCESS is returned.

    int  tm_thread_suspend(int thread_id);

    This function suspend the previously created thread specified by
    thread_id. If successful, a TM_SUCCESS is returned.

    void  tm_thread_relinquish(void);

    This function lets all other threads of same priority execute
    before the calling thread runs again.

    void  tm_thread_sleep(int seconds);

    This function suspends the calling thread for the specified
    number of seconds.

    int  tm_queue_create(int queue_id);

    This function creates a queue with a capacity to hold at least
    one 16-byte message. If successful, a TM_SUCCESS is returned.

    int  tm_queue_send(int queue_id, unsigned long *message_ptr);

    This function sends a message to the previously created queue.
    If successful, a TM_SUCCESS is returned.

    int  tm_queue_receive(int queue_id, unsigned long *message_ptr);

    This function receives a message from the previously created
    queue. If successful, a TM_SUCCESS is returned.

    int  tm_semaphore_create(int semaphore_id);

    This function creates a binary semaphore. If successful, a
    TM_SUCCESS is returned.

    int  tm_semaphore_get(int semaphore_id);

    This function gets the previously created binary semaphore.
    If successful, a TM_SUCCESS is returned.

    int  tm_semaphore_put(int semaphore_id);

    This function puts the previously created binary semaphore.
    If successful, a TM_SUCCESS is returned.

    int  tm_memory_pool_create(int pool_id);

    This function creates a memory pool able to satisfy at least one
    128-byte block of memory. If successful, a TM_SUCCESS is returned.

    int  tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr);

    This function allocates a 128-byte block of memory from the
    previously created memory pool. If successful, a TM_SUCCESS
    is returned along with the pointer to the allocated memory
    in the "memory_ptr" variable.

    int  tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr);

    This function releases the previously allocated 128-byte block
    of memory. If successful, a TM_SUCCESS is returned.


2.2 Porting Requirements Checklist

The following requirements are made in order to ensure fair benchmarks
are achieved on each RTOS performing the test:

    1. Time period should be 30 seconds. This will ensure the printf
       processing in the reporting thread is insignificant.

    *  Zephyr : Requirement met.

    2. The porting layer services are implemented inside of
       tm_porting_layer_[RTOS].c and NOT as macros.

    *  Zephyr : Requirements met.

    3. The tm_thread_sleep service is implemented by a 10ms RTOS
       periodic interrupt source.

    *  Zephyr : Requirement met. System tick rate = 100 Hz.

    4. Locking regions of the tests and/or the RTOS in cache is
       not allowed.

    *  Zephyr : Requirement met.

    5. The Interrupt Processing and Interrupt Preemption Processing tests
       require an instruction that generates an interrupt. Please refer
       to tm_porting_layer.h for an example implementation.

    *  Zephyr : Requirement met. See irq_offload().
