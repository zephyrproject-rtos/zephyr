.. _nanokernel_example:

Semaphore, Timer, and Fiber Example
###################################

The following example is pulled from the file:
:file:`samples/microkernel/apps/hello_world/src/hello.c`.

Example Code
************

.. code-block:: c

   #include <nanokernel.h>

   #include <nanokernel/cpu.h>

   /* specify delay between greetings (in ms); compute equivalent in ticks */

   #define SLEEPTIME

   #define SLEEPTICKS (SLEEPTIME * CONFIG_TICKFREQ / 1000)

   #define STACKSIZE 2000

   char fiberStack[STACKSIZE];

   struct nano_sem nanoSemTask;

   struct nano_sem nanoSemFiber;

   void fiberEntry (void)

   {
      struct nano_timer timer;
      uint32_t data[2] = {0, 0};

      nano_sem_init (&nanoSemFiber);
      nano_timer_init (&timer, data);

      while (1)
      {
         /* wait for task to let us have a turn */
         nano_fiber_sem_take_wait (&nanoSemFiber);

         /* say "hello" */
         PRINT ("%s: Hello World!\n", __FUNCTION__);

         /* wait a while, then let task have a turn */
         nano_fiber_timer_start (&timer, SLEEPTICKS);
         nano_fiber_timer_wait (&timer);
         nano_fiber_sem_give (&nanoSemTask);
      }

   }

   void main (void)

   {
      struct nano_timer timer;
      uint32_t data[2] = {0, 0};

      task_fiber_start (&fiberStack[0], STACKSIZE,
                     (nano_fiber_entry_t) fiberEntry, 0, 0, 7, 0);

      nano_sem_init (&nanoSemTask);
      nano_timer_init (&timer, data);

      while (1)
      {
         /* say "hello" */
         PRINT ("%s: Hello World!\n", __FUNCTION__);

         /* wait a while, then let fiber have a turn */
         nano_task_timer_start (&timer, SLEEPTICKS);
         nano_task_timer_wait (&timer);
         nano_task_sem_give (&nanoSemFiber);

         /* now wait for fiber to let us have a turn */
         nano_task_sem_take_wait (&nanoSemTask);
      }

   }

Step-by-Step Description
************************

A quick breakdown of the major objects in use by this sample includes:

- One fiber, executing in the :c:func:`fiberEntry()` routine.

- The background task, executing in the :c:func:`main()` routine.

- Two semaphores (*nanoSemTask*, *nanoSemFiber*),

- Two timers:

   + One local to the fiber (timer)

   + One local to background task (timer)

First, the background task starts executing main(). The background task
calls task_fiber_start initializing and starting the fiber. Since a
fiber is available to be run, the background task is pre-empted and the
fiber begins running.

Execution jumps to fiberEntry. nanoSemFiber and the fiber-local timer
before dropping into the while loop, where it takes and waits on
nanoSemFiber. task_fiber_start.

The background task initializes nanoSemTask and the task-local timer.

The following steps repeat endlessly:

#. The background task execution begins at the top of the main while
   loop and prints, “main: Hello World!”

#. The background task then starts a timer for SLEEPTICKS in the
   future, and waits for that timer to expire.


#. Once the timer expires, it signals the fiber by giving the
   nanoSemFiber semaphore, which in turn marks the fiber as runnable.

#. The fiber, now marked as runnable, pre-empts the background
   process, allowing execution to jump to the fiber.
   nano_fiber_sem_take_wait.

#. The fiber then prints, “fiberEntry: Hello World!” It starts a time
   for SLEEPTICKS in the future and waits for that timer to expire. The
   fiber is marked as not runnable, and execution jumps to the
   background task.

#. The background task then takes and waits on the nanoSemTask
   semaphore.

#. Once the timer expires, the fiber signals the background task by
   giving the nanoSemFiber semaphore. The background task is marked as
   runnable, but code execution continues in the fiber, since fibers
   take priority over the background task. The fiber execution
   continues to the top of the while loop, where it takes and waits on
   nanoSemFiber. The fiber is marked as not runnable, and the
   background task is scheduled.

#. The background task execution picks up after the call to
   :c:func:`nano_task_sem_take_wait()`. It jumps to the top of the
   while loop.