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

    pthread_atfork(),yes
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
    pthread_cleanup_pop(),yes
    pthread_cleanup_push(),yes
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
    pthread_setcanceltype(),yes
    pthread_setspecific(),yes
    pthread_sigmask(),yes
    pthread_testcancel(),yes

.. _posix_option_group_posix_threads_ext:

POSIX_THREADS_EXT
=================

This table lists service support status in Zephyr:

.. csv-table:: POSIX_THREADS_EXT
   :header: API, Supported
   :widths: 50,10

    pthread_attr_getguardsize(),yes
    pthread_attr_setguardsize(),yes
    pthread_mutexattr_gettype(),yes
    pthread_mutexattr_settype(),yes

.. _posix_option_group_xsi_threads_ext:

XSI_THREADS_EXT
===============

The XSI_THREADS_EXT option group is required because it provides
functions to control a thread's stack. This is considered useful for any
real-time application.

This table lists service support status in Zephyr:

.. csv-table:: XSI_THREADS_EXT
   :header: API, Supported
   :widths: 50,10

    pthread_attr_getstack(),yes
    pthread_attr_setstack(),yes
    pthread_getconcurrency(),yes
    pthread_setconcurrency(),yes

.. _posix_option_group_c_lang_jump:

POSIX_C_LANG_JUMP
=================

The ``POSIX_C_LANG_JUMP`` Option Group is included in the ISO C standard.

.. note::
   When using Newlib, Picolibc, or other C libraries conforming to the ISO C Standard, the
   ``POSIX_C_LANG_JUMP`` Option Group is considered supported.

.. csv-table:: POSIX_C_LANG_JUMP
   :header: API, Supported
   :widths: 50,10

    setjmp(), yes
    longjmp(), yes

.. _posix_option_group_c_lang_math:

POSIX_C_LANG_MATH
=================

The ``POSIX_C_LANG_MATH`` Option Group is included in the ISO C standard.

.. note::
   When using Newlib, Picolibc, or other C libraries conforming to the ISO C Standard, the
   ``POSIX_C_LANG_MATH`` Option Group is considered supported.

Please refer to `Subprofiling Considerations`_ for details on the ``POSIX_C_LANG_MATH`` Option
Group.

.. _posix_option_group_c_lang_support:

POSIX_C_LANG_SUPPORT
====================

The POSIX_C_LANG_SUPPORT option group contains the general ISO C Library.

.. note::
   When using Newlib, Picolibc, or other C libraries conforming to the ISO C Standard, the entire
   ``POSIX_C_LANG_SUPPORT`` Option Group is considered supported.

Please refer to `Subprofiling Considerations`_ for details on the ``POSIX_C_LANG_SUPPORT`` Option
Group.

For more information on developing Zephyr applications in the C programming language, please refer
to :ref:`details<language_support>`.

.. _posix_option_group_realtime_signals:

POSIX_REALTIME_SIGNALS
======================

.. csv-table:: POSIX_REALTIME_SIGNALS
   :header: API, Supported
   :widths: 50,10

    sigqueue(),
    sigtimedwait(),
    sigwaitinfo(),

.. _posix_option_group_signal_jump:

POSIX_SIGNAL_JUMP
=================

.. csv-table:: POSIX_SIGNAL_JUMP
   :header: API, Supported
   :widths: 50,10

    siglongjmp(),
    sigsetjmp(),

.. _posix_option_group_single_process:

POSIX_SINGLE_PROCESS
====================

The POSIX_SINGLE_PROCESS option group contains services for single
process applications.

.. csv-table:: POSIX_SINGLE_PROCESS
   :header: API, Supported
   :widths: 50,10

    confstr(),yes
    environ,yes
    errno,yes
    getenv(),yes
    setenv(),yes
    sysconf(),yes
    uname(),yes
    unsetenv(),yes

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
    sigprocmask(),yes
    sigsuspend(),
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

.. _posix_option_group_file_system:

POSIX_FILE_SYSTEM
=================

.. csv-table:: POSIX_FILE_SYSTEM
   :header: API, Supported
   :widths: 50,10

    access(),
    chdir(),
    closedir(), yes
    creat(),
    fchdir(),
    fpathconf(),
    fstat(),
    fstatvfs(),
    getcwd(),
    link(),
    mkdir(), yes
    mkstemp(),
    opendir(), yes
    pathconf(),
    readdir(), yes
    remove(),
    rename(), yes
    rewinddir(),
    rmdir(),
    stat(), yes
    statvfs(),
    tmpfile(),
    tmpnam(),
    truncate(),
    unlink(), yes
    utime(),

.. _posix_option_group_networking:

POSIX_NETWORKING
================

.. csv-table:: POSIX_NETWORKING
   :header: API, Supported
   :widths: 50,10

    accept(),yes
    bind(),yes
    connect(),yes
    endhostent(),yes
    endnetent(),yes
    endprotoent(),yes
    endservent(),yes
    freeaddrinfo(),yes
    gai_strerror(),yes
    getaddrinfo(),yes
    gethostent(),yes
    gethostname(),yes
    getnameinfo(),yes
    getnetbyaddr(),yes
    getnetbyname(),yes
    getnetent(),yes
    getpeername(),yes
    getprotobyname(),yes
    getprotobynumber(),yes
    getprotoent(),yes
    getservbyname(),yes
    getservbyport(),yes
    getservent(),yes
    getsockname(),yes
    getsockopt(),yes
    htonl(),yes
    htons(),yes
    if_freenameindex(),yes
    if_indextoname(),yes
    if_nameindex(),yes
    if_nametoindex(),yes
    inet_addr(),yes
    inet_ntoa(),yes
    inet_ntop(),yes
    inet_pton(),yes
    listen(),yes
    ntohl(),yes
    ntohs(),yes
    recv(),yes
    recvfrom(),yes
    recvmsg(),yes
    send(),yes
    sendmsg(),yes
    sendto(),yes
    sethostent(),yes
    setnetent(),yes
    setprotoent(),yes
    setservent(),yes
    setsockopt(),yes
    shutdown(),yes
    socket(),yes
    sockatmark(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    socketpair(),yes

.. _posix_option_group_pipe:

POSIX_PIPE
==========

.. csv-table:: POSIX_PIPE
   :header: API, Supported
   :widths: 50,10

    pipe(),

.. _posix_option_group_semaphores:

POSIX_SEMAPHORES
================

.. csv-table:: POSIX_SEMAPHORES
   :header: API, Supported
   :widths: 50,10

    sem_close(),yes
    sem_destroy(),yes
    sem_getvalue(),yes
    sem_init(),yes
    sem_open(),yes
    sem_post(),yes
    sem_trywait(),yes
    sem_unlink(),yes
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

    clock_getres(),yes
    clock_gettime(),yes
    clock_settime(),yes
    nanosleep(),yes
    timer_create(),yes
    timer_delete(),yes
    timer_gettime(),yes
    timer_getoverrun(),yes
    timer_settime(),yes

.. _posix_option_group_fd_mgmt:

POSIX_FD_MGMT
=============

This table lists service support status in Zephyr for `POSIX_FD_MGMT`:

.. csv-table:: POSIX_FD_MGMT
   :header: API, Supported
   :widths: 50,10

    dup(),
    dup2(),
    fcntl(),
    fgetpos(),
    fseek(),
    fseeko(),
    fsetpos(),
    ftell(),
    ftello(),
    ftruncate(),yes
    lseek(),
    rewind(),

.. _posix_option_group_file_locking:

POSIX_FILE_LOCKING
==================

This table lists service support status in Zephyr for `POSIX_FD_MGMT`:

.. csv-table:: POSIX_FILE_LOCKING
   :header: API, Supported
   :widths: 50,10

    flockfile(),
    ftrylockfile(),
    funlockfile(),
    getc_unlocked(),
    getchar_unlocked(),
    putc_unlocked(),
    putchar_unlocked(),

.. _posix_options:

Additional POSIX Options
========================

.. _posix_option_asynchronous_io:

_POSIX_ASYNCHRONOUS_IO
++++++++++++++++++++++

.. csv-table:: _POSIX_ASYNCHRONOUS_IO
   :header: API, Supported
   :widths: 50,10

    aio_cancel(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    aio_error(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    aio_fsync(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    aio_read(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    aio_return(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    aio_suspend(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    aio_write(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    lio_listio(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)

.. _posix_option_fsync:

_POSIX_FSYNC
++++++++++++

.. csv-table:: _POSIX_FSYNC
   :header: API, Supported
   :widths: 50,10

    fsync(),yes

.. _posix_option_memlock:

_POSIX_MEMLOCK
++++++++++++++

.. csv-table:: _POSIX_MEMLOCK
   :header: API, Supported
   :widths: 50,10

    mlockall(),
    munlockall(),

.. _posix_option_memlock_range:

_POSIX_MEMLOCK_RANGE
++++++++++++++++++++

.. csv-table:: _POSIX_MEMLOCK_RANGE
   :header: API, Supported
   :widths: 50,10

    mlock(),
    munlock(),

.. _posix_option_message_passing:

_POSIX_MESSAGE_PASSING
++++++++++++++++++++++

.. csv-table:: _POSIX_MESSAGE_PASSING
   :header: API, Supported
   :widths: 50,10

    mq_close(),yes
    mq_getattr(),yes
    mq_notify(),yes
    mq_open(),yes
    mq_receive(),yes
    mq_send(),yes
    mq_setattr(),yes
    mq_unlink(),yes

.. _posix_option_monotonic_clock:

_POSIX_MONOTONIC_CLOCK
++++++++++++++++++++++

.. csv-table:: _POSIX_MONOTONIC_CLOCK
   :header: API, Supported
   :widths: 50,10

    CLOCK_MONOTONIC,yes

.. _posix_option_priority_scheduling:

_POSIX_PRIORITY_SCHEDULING
++++++++++++++++++++++++++

.. csv-table:: _POSIX_PRIORITY_SCHEDULING
   :header: API, Supported
   :widths: 50,10

    sched_get_priority_max(),yes
    sched_get_priority_min(),yes
    sched_getparam(),yes
    sched_getscheduler(),yes
    sched_rr_get_interval(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    sched_setparam(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    sched_setscheduler(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
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
    pthread_rwlockattr_getpshared(),yes
    pthread_rwlockattr_init(),yes
    pthread_rwlockattr_setpshared(),yes

.. _posix_shared_memory_objects:

_POSIX_SHARED_MEMORY_OBJECTS
++++++++++++++++++++++++++++

.. csv-table:: _POSIX_SHARED_MEMORY_OBJECTS
   :header: API, Supported
   :widths: 50,10

    mmap(),
    munmap(),
    shm_open(),
    shm_unlink(),

.. _posix_option_synchronized_io:

_POSIX_SYNCHRONIZED_IO
++++++++++++++++++++++

.. csv-table:: _POSIX_SYNCHRONIZED_IO
   :header: API, Supported
   :widths: 50,10

    fdatasync(),
    fsync(),yes
    msync(),

.. _posix_option_thread_attr_stackaddr:

_POSIX_THREAD_ATTR_STACKADDR
++++++++++++++++++++++++++++

.. csv-table:: _POSIX_THREAD_ATTR_STACKADDR
   :header: API, Supported
   :widths: 50,10

    pthread_attr_getstackaddr(),yes
    pthread_attr_setstackaddr(),yes

.. _posix_option_thread_cputime:

_POSIX_THREAD_CPUTIME
+++++++++++++++++++++

.. csv-table:: _POSIX_THREAD_CPUTIME
   :header: API, Supported
   :widths: 50,10

    CLOCK_THREAD_CPUTIME_ID,yes
    pthread_getcpuclockid(),yes

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

    pthread_attr_getinheritsched(),yes
    pthread_attr_getschedpolicy(),yes
    pthread_attr_getscope(),yes
    pthread_attr_setinheritsched(),yes
    pthread_attr_setschedpolicy(),yes
    pthread_attr_setscope(),yes
    pthread_getschedparam(),yes
    pthread_setschedparam(),yes
    pthread_setschedprio(),yes

.. _posix_thread_safe_functions:

_POSIX_THREAD_SAFE_FUNCTIONS
++++++++++++++++++++++++++++

.. csv-table:: _POSIX_THREAD_SAFE_FUNCTIONS
    :header: API, Supported
    :widths: 50,10

    asctime_r(),
    ctime_r(),
    flockfile(),
    ftrylockfile(),
    funlockfile(),
    getc_unlocked(), yes
    getchar_unlocked(), yes
    getgrgid_r(),
    getgrnam_r(),
    getpwnam_r(),
    getpwuid_r(),
    gmtime_r(), yes
    localtime_r(),
    putc_unlocked(), yes
    putchar_unlocked(), yes
    rand_r(), yes
    readdir_r(),
    strerror_r(), yes
    strtok_r(), yes

.. _posix_option_timeouts:

_POSIX_TIMEOUTS
+++++++++++++++

.. csv-table:: _POSIX_TIMEOUTS
   :header: API, Supported
   :widths: 50,10

    mq_timedreceive(),yes
    mq_timedsend(),yes
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

    fattach(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    fdetach(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    getmsg(),  yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    getpmsg(),  yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    ioctl(),yes
    isastream(),yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    putmsg(), yes (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    putpmsg(),

.. _Subprofiling Considerations:
    https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_subprofiles.html
