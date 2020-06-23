The .cpp files in this suite should eventually be merged into tests/application_development/libcxx/src/threads/.

The reason that it must be here is because out toolchain does not currently enable C++ threads (although we should be able to using pthreads soon).

The main things missing from pthreads that are required for the regular libstdc++ bits/gthr-posix.h to work are:

1) pthread_create() allowing NULL attribute parameter
2) modification of bits/gthr-posix.h to not use PTHREAD_MUTEX_INITIALIZER, but to actually call pthread_mutex_init()
3) modification of bits/gthr-posix.h to not use PTHREAD_COND_INITIALIZER, but to actually call pthread_cond_init()

Until our toolchain makes the necessary changes, these tests may only be run under BOARD=native_posix_64 (maybe BOARD=native_posix_32?) using the "whitelist" facility.
