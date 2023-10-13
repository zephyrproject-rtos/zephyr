.. _posix_option_groups:

Subprofiling Option Groups
##########################

.. _posix_option_group_threads_base:

POSIX_THREADS_BASE
==================

The basic assumption in this profile is that the system
consists of a single (implicit) process with multiple threads. Therefore, the
standard requires all basic thread services, except those related to
multiple processes.

.. csv-table:: POSIX_THREADS_BASE
   :header: API, Supported
   :widths: 50,10

    pthread_atfork(),
    pthread_attr_destroy(),yes
    pthread_attr_getdetachstate(),yes
    pthread_attr_getschedparam(),yes
    pthread_attr_init(),yes
    pthread_attr_setdetachstate(),yes
    pthread_attr_setschedparam(),yes
    pthread_barrier_destroy(),yes
    pthread_barrier_init(),yes
    pthread_barrier_wait(),yes
    pthread_barrierattr_destroy(),yes
    pthread_barrierattr_getpshared(),yes
    pthread_barrierattr_init(),yes
    pthread_barrierattr_setpshared(),yes
    pthread_cancel(),yes
    pthread_cleanup_pop(),
    pthread_cleanup_push(),
    pthread_cond_broadcast(),yes
    pthread_cond_destroy(),yes
    pthread_cond_init(),yes
    pthread_cond_signal(),yes
    pthread_cond_timedwait(),yes
    pthread_cond_wait(),yes
    pthread_condattr_destroy(),yes
    pthread_condattr_init(),yes
    pthread_create(),yes
    pthread_detach(),yes
    pthread_equal(),yes
    pthread_exit(),yes
    pthread_getspecific(),yes
    pthread_join(),yes
    pthread_key_create(),yes
    pthread_key_delete(),yes
    pthread_kill(),
    pthread_mutex_destroy(),yes
    pthread_mutex_init(),yes
    pthread_mutex_lock(),yes
    pthread_mutex_trylock(),yes
    pthread_mutex_unlock(),yes
    pthread_mutexattr_destroy(),yes
    pthread_mutexattr_init(),yes
    pthread_once(),yes
    pthread_self(),yes
    pthread_setcancelstate(),yes
    pthread_setcanceltype(),
    pthread_setspecific(),yes
    pthread_sigmask(),
    pthread_testcancel(),

.. _posix_option_group_xsi_thread_ext:

XSI_THREAD_EXT
==============

The XSI_THREADS_EXT option group is required because it provides
functions to control a thread's stack. This is considered useful for any
real-time application.

This table lists service support status in Zephyr:

.. csv-table:: XSI_THREAD_EXT
   :header: API, Supported
   :widths: 50,10

    pthread_attr_getguardsize(),
    pthread_attr_getstack(),yes
    pthread_attr_setguardsize(),
    pthread_attr_setstack(),yes
    pthread_getconcurrency(),
    pthread_setconcurrency()

.. _posix_option_group_c_lang_support:

POSIX_C_LANG_SUPPORT
====================

The POSIX_C_LANG_SUPPORT option group contains the general ISO C
Library.

.. csv-table:: POSIX_C_LANG_SUPPORT
   :header: API, Supported
   :widths: 50,10

    abs(),yes
    asctime(),
    asctime_r(),
    atof(),
    atoi(),yes
    atol(),
    atoll(),
    bsearch(),yes
    calloc(),yes
    ctime(),
    ctime_r(),
    difftime(),
    div(),
    feclearexcept(),
    fegetenv(),
    fegetexceptflag(),
    fegetround(),
    feholdexcept(),
    feraiseexcept(),
    fesetenv(),
    fesetexceptflag(),
    fesetround(),
    fetestexcept(),
    feupdateenv(),
    free(),yes
    gmtime(),yes
    gmtime_r(),yes
    imaxabs(),
    imaxdiv(),
    isalnum(),yes
    isalpha(),yes
    isblank(),
    iscntrl(),yes
    isdigit(),yes
    isgraph(),yes
    islower(),
    isprint(),yes
    ispunct(),
    isspace(),yes
    isupper(),yes
    isxdigit(),yes
    labs(),yes
    ldiv(),
    llabs(),yes
    lldiv(),
    localeconv(),
    localtime(),yes
    localtime_r(),
    malloc(),yes
    memchr(),yes
    memcmp(),yes
    memcpy(),yes
    memmove(),yes
    memset(),yes
    mktime(),yes
    qsort(),yes
    rand(),yes
    rand_r(),yes
    realloc(),yes
    setlocale(),
    snprintf(),yes
    sprintf(),yes
    srand(),yes
    sscanf(),
    strcat(),yes
    strchr(),yes
    strcmp(),yes
    strcoll(),
    strcpy(),yes
    strcspn(),yes
    strerror(),yes
    strerror_r(),yes
    strftime(),
    strlen(),yes
    strncat(),yes
    strncmp(),yes
    strncpy(),yes
    strpbrk(),
    strrchr(),yes
    strspn(),yes
    strstr(),yes
    strtod(),
    strtof(),
    strtoimax(),
    strtok(),yes
    strtok_r(),yes
    strtol(),yes
    strtold(),
    strtoll(),yes
    strtoul(),yes
    strtoull(),yes
    strtoumax(),
    strxfrm(),
    time(),yes
    tolower(),yes
    toupper(),yes
    tzname(),
    tzset(),
    va_arg(),yes
    va_copy(),yes
    va_end(),yes
    va_start(),yes
    vsnprintf(),yes
    vsprintf(),yes
    vsscanf(),

.. _posix_option_group_single_process:

POSIX_SINGLE_PROCESS
====================

The POSIX_SINGLE_PROCESS option group contains services for single
process applications.

.. csv-table:: POSIX_SINGLE_PROCESS
   :header: API, Supported
   :widths: 50,10

    confstr(),
    environ,
    errno,yes
    getenv(),
    setenv(),
    sysconf(),
    uname(),yes
    unsetenv()

.. _posix_option_group_signals:

POSIX_SIGNALS
=============

Signal services are a basic mechanism within POSIX-based systems and are
required for error and event handling.

.. csv-table:: POSIX_SIGNALS
   :header: API, Supported
   :widths: 50,10

    abort(),yes
    alarm(),
    kill(),
    pause(),
    raise(),
    sigaction(),
    sigaddset(),yes
    sigdelset(),yes
    sigemptyset(),yes
    sigfillset(),yes
    sigismember(),yes
    signal(),
    sigpending(),
    sigprocmask(),
    igsuspend(),
    sigwait(),
    strsignal(),yes

.. _posix_option_group_device_io:

POSIX_DEVICE_IO
===============

.. csv-table:: POSIX_DEVICE_IO
   :header: API, Supported
   :widths: 50,10

    FD_CLR(),yes
    FD_ISSET(),yes
    FD_SET(),yes
    FD_ZERO(),yes
    clearerr(),yes
    close(),yes
    fclose(),
    fdopen(),
    feof(),
    ferror(),
    fflush(),
    fgetc(),
    fgets(),
    fileno(),
    fopen(),
    fprintf(),yes
    fputc(),yes
    fputs(),yes
    fread(),
    freopen(),
    fscanf(),
    fwrite(),yes
    getc(),
    getchar(),
    gets(),
    open(),yes
    perror(),yes
    poll(),yes
    printf(),yes
    pread(),
    pselect(),
    putc(),yes
    putchar(),yes
    puts(),yes
    pwrite(),
    read(),yes
    scanf(),
    select(),yes
    setbuf(),
    setvbuf(),
    stderr,
    stdin,
    stdout,
    ungetc(),
    vfprintf(),yes
    vfscanf(),
    vprintf(),yes
    vscanf(),
    write(),yes

.. _posix_option_group_barriers:

POSIX_BARRIERS
==============

.. csv-table:: POSIX_BARRIERS
   :header: API, Supported
   :widths: 50,10

    pthread_barrier_destroy(),yes
    pthread_barrier_init(),yes
    pthread_barrier_wait(),yes
    pthread_barrierattr_destroy(),yes
    pthread_barrierattr_init(),yes

.. _posix_option_group_clock_selection:

POSIX_CLOCK_SELECTION
=====================

.. csv-table:: POSIX_CLOCK_SELECTION
   :header: API, Supported
   :widths: 50,10

    pthread_condattr_getclock(),yes
    pthread_condattr_setclock(),yes
    clock_nanosleep(),yes

.. _posix_option_group_semaphores:

POSIX_SEMAPHORES
================

.. csv-table:: POSIX_SEMAPHORES
   :header: API, Supported
   :widths: 50,10

    sem_close(),
    sem_destroy(),yes
    sem_getvalue(),yes
    sem_init(),yes
    sem_open(),
    sem_post(),yes
    sem_trywait(),yes
    sem_unlink(),
    sem_wait(),yes

.. _posix_option_group_spin_locks:

POSIX_SPIN_LOCKS
================

.. csv-table:: POSIX_SPIN_LOCKS
   :header: API, Supported
   :widths: 50,10

    pthread_spin_destroy(),yes
    pthread_spin_init(),yes
    pthread_spin_lock(),yes
    pthread_spin_trylock(),yes
    pthread_spin_unlock(),yes

.. _posix_option_group_timers:

POSIX_TIMERS
============

.. csv-table:: POSIX_TIMERS
   :header: API, Supported
   :widths: 50,10

    clock_getres(),
    clock_gettime(),yes
    clock_settime(),yes
    nanosleep(),yes
    timer_create(),yes
    timer_delete(),yes
    timer_gettime(),yes
    timer_getoverrun(),yes
    timer_settime(),yes


.. _posix_options:

Additional POSIX Options
========================

.. _posix_option_message_passing:

_POSIX_MESSAGE_PASSING
++++++++++++++++++++++

.. csv-table:: _POSIX_MESSAGE_PASSING
   :header: API, Supported
   :widths: 50,10

    mq_close(),yes
    mq_getattr(),yes
    mq_notify(),
    mq_open(),yes
    mq_receive(),yes
    mq_send(),yes
    mq_setattr(),yes
    mq_unlink(),yes

_POSIX_PRIORITY_SCHEDULING
++++++++++++++++++++++++++

.. _posix_option_priority_scheduling:

.. csv-table:: _POSIX_PRIORITY_SCHEDULING
   :header: API, Supported
   :widths: 50,10

    sched_get_priority_max(),yes
    sched_get_priority_min(),yes
    sched_getparam(),
    sched_getscheduler(),
    sched_rr_get_interval(),
    sched_setparam(),
    sched_setscheduler(),
    sched_yield(),yes

.. _posix_option_reader_writer_locks:

_POSIX_READER_WRITER_LOCKS
++++++++++++++++++++++++++

.. csv-table:: _POSIX_READER_WRITER_LOCKS
   :header: API, Supported
   :widths: 50,10

    pthread_rwlock_destroy(),yes
    pthread_rwlock_init(),yes
    pthread_rwlock_rdlock(),yes
    pthread_rwlock_tryrdlock(),yes
    pthread_rwlock_trywrlock(),yes
    pthread_rwlock_unlock(),yes
    pthread_rwlock_wrlock(),yes
    pthread_rwlockattr_destroy(),yes
    pthread_rwlockattr_getpshared(),
    pthread_rwlockattr_init(),yes
    pthread_rwlockattr_setpshared(),

.. _posix_option_thread_attr_stackaddr:

_POSIX_THREAD_ATTR_STACKADDR
++++++++++++++++++++++++++++

.. csv-table:: _POSIX_THREAD_ATTR_STACKADDR
   :header: API, Supported
   :widths: 50,10

    pthread_attr_getstackaddr(),yes
    pthread_attr_setstackaddr(),yes

.. _posix_option_thread_attr_stacksize:

_POSIX_THREAD_ATTR_STACKSIZE
++++++++++++++++++++++++++++

.. csv-table:: _POSIX_THREAD_ATTR_STACKSIZE
   :header: API, Supported
   :widths: 50,10

    pthread_attr_getstacksize(),yes
    pthread_attr_setstacksize(),yes

.. _posix_option_thread_priority_scheduling:

_POSIX_THREAD_PRIORITY_SCHEDULING
+++++++++++++++++++++++++++++++++

.. csv-table:: _POSIX_THREAD_PRIORITY_SCHEDULING
   :header: API, Supported
   :widths: 50,10

    pthread_attr_getinheritsched(),
    pthread_attr_getschedpolicy(),yes
    pthread_attr_getscope(),
    pthread_attr_setinheritsched(),
    pthread_attr_setschedpolicy(),yes
    pthread_attr_setscope(),
    pthread_getschedparam(),yes
    pthread_setschedparam(),yes
    pthread_setschedprio(),yes

.. _posix_option_timeouts:

_POSIX_TIMEOUTS
+++++++++++++++

.. csv-table:: _POSIX_TIMEOUTS
   :header: API, Supported
   :widths: 50,10

    mq_timedreceive(),
    mq_timedsend(),
    pthread_mutex_timedlock(),yes
    pthread_rwlock_timedrdlock(),yes
    pthread_rwlock_timedwrlock(),yes
    sem_timedwait(),yes
    posix_trace_timedgetnext_event(),

.. _posix_option_xopen_streams:

_XOPEN_STREAMS
++++++++++++++

.. csv-table:: _XOPEN_STREAMS
   :header: API, Supported
   :widths: 50,10

    fattach(),
    fdetach(),
    getmsg(),
    getpmsg(),
    ioctl(),yes
    isastream(),
    putmsg(),
    putpmsg(),
