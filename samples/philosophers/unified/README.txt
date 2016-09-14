Title: Dining Philosophers

Description:

An implementation of a solution to the Dining Philosophers problem
(a classic multi-thread synchronization problem).  This particular
implementation demonstrates the usage of nanokernel semaphores and
timers from multiple (6) fibers.

The philosopher always tries to get the lowest fork first (f1 then f2).  When
done, he will give back the forks in the reverse order (f2 then f1).  If he
gets two forks, he is EATING.  Otherwise, he is THINKING. Transitional states
are shown as well, such as STARVING when the philosopher is hungry but the
forks are not available, and HOLDING ONE FORK when a philosopher is waiting
for the second fork to be available.

Each Philosopher will randomly alternate between the EATING and THINKING state.

--------------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make qemu

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

Philosopher 0 [P: 3]    HOLDING ONE FORK
Philosopher 1 [P: 2]    HOLDING ONE FORK
Philosopher 2 [P: 1]   EATING  [ 1900 ms ]
Philosopher 3 [P: 0]  THINKING [ 2500 ms ]
Philosopher 4 [C:-1]  THINKING [ 2200 ms ]
Philosopher 5 [C:-2]  THINKING [ 1700 ms ]


Demo Description
----------------
An implementation of a solution to the Dining Philosophers
problem (a classic multi-thread synchronization problem).
This particular implementation demonstrates the usage of multiple
preemptible and cooperative threads of differing priorities, as
well as dynamic mutexes and thread sleeping.
