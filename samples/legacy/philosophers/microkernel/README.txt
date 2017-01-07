Title: Dining Philosophers

Description:

An implementation of a solution to the Dining Philosophers problem
(a classic multi-thread synchronization problem).  This particular
implementation demonstrates the usage of microkernel task groups,
Mutex APIs and timer drivers from multiple (6) tasks.

The philosopher always tries to get the lowest fork first (f1 then f2).
When done, he will give back the forks in the reverse order (f2 then f1).
If he gets two forks, he is EATING.  Otherwise, he is THINKING.

Each philosopher will randomly alternate between the EATING and THINKING
states.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
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

Philosopher 0 EATING
Philosopher 1 THINKING
Philosopher 2 THINKING
Philosopher 3 EATING
Philosopher 4 THINKING
Philosopher 5 THINKING








Demo Description
----------------
An implementation of a solution to the Dining Philosophers problem
(a classic multi-thread synchronization problem).  This particular
implementation demonstrates the usage of multiple (6) tasks
of differing priorities and the nanokernel semaphores and timers.
