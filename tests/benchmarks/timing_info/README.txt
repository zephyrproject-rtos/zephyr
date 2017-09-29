Title: Timing Information

Description:

Timing measurements for the following features of the OS.
1. Context switch
   Time taken to compete the context switch, i.e time spent in _Swap function.
2. Interrupt latency
   Time taken from the start of the common interrupt handler till the
   actual ISR handler being called.
3. Tick overhead
   Time spent by the cpu in the tick handler.
4. Thread Creation
   Time spent in creating a thread.
5. Thread cancel
   Time taken to cancel the thread which is not yet started execution.
   So the time taken to complete the function call is measured.
6. Thread abort
   Time taken to abort the thread which has already started execution.
   So the time measured is from the start of the function call until the
   another thread is swapped in.
7. Thread Suspend
   The time measured is from the start of the function call until the current
   thread is suspended and another thread is swapped in.
8. Thread Resume
   The time measured is from the start of the function call until the required
   thread is resumed by swap operation.
9. Thread Yield
   The time measured is from the start of the function call until the higher priority
   thread is swapped in.
10. Thread Sleep
   The time measured is from the start of the function call until the current
   thread is put on the timeout queue and another thread is swapped in.
11. Heap Malloc
    The time to allocate heap memory in fixed size chunks. Continuously allocate
    the memory from the pool. Average time taken to complete the function call
    is measured.
12. Heap Free
    Time to free heap memory in fixed size chunks. Continuously free
    the memory that was allocated. Average time taken to complete the function call
    is measured.
13. Semaphore Take with context switch
    Taking a semaphore causes a thread waiting on the semaphore to be swapped in.
    Thus Time measured is the time taken from the function call till the waiting
    thread is swapped in.
14. Semaphore Give with context switch
    Giving a semaphore causes a thread waiting on the semaphore to be swapped
    in (higher priority).
    Thus Time measured is the time taken from the function call till the waiting
    thread is swapped in.
15. Semaphore Take without context switch
    Time taken to take the semaphore. Thus time to complete the function
    call is measured.
16. Semaphore Give without context switch
    Time taken to give the semaphore. Thus time to complete the function
    call is measured.
17. Mutex lock
    Time taken to lock the mutex. Thus time to complete the function
    call is measured.
18. Mutex unlock
    Time taken to unlock the mutex. Thus time to complete the function
    call is measured.
19. Message Queue Put with context switch
    A thread is waiting for a message to arrive. The time taken from the start
    of the function call till the waiting thread is swapped in is measured.
20. Message Queue Put without context switch
    The time taken to complete the function call is measured.
21. Message Queue get with context switch
    A thread has gone into waiting because the message queue is full.
    When a get occurs this thread gets free to execute. The time taken from
    the start of the function call till the waiting thread is
    swapped in is measured.
22. Message Queue get without context switch
    The time taken to complete the function call is measured.
23. MailBox synchronous put
    The time taken from the start of the function call till the waiting thread
    is swapped in is measured.
24. MailBox synchronous get
    The time taken from the start of the function call till the waiting thread
    is swapped in is measured.
25. MailBox asynchronous put
    The time taken to complete the function call is measured.
26. MailBox get without context switch
    The time taken to complete the function call is measured.


--------------------------------------------------------------------------------

Building and Running Project:

This benchmark outputs to the console.  It can be built and executed
on QEMU as follows:

    make run

--------------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

--------------------------------------------------------------------------------

Sample Output:
