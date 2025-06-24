POSIX Option and Option Group Details
#####################################

.. _posix_option_groups:

POSIX Option Groups
===================

.. _posix_option_group_barriers:

POSIX_BARRIERS
++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_BARRIERS`.

.. csv-table:: POSIX_BARRIERS
   :header: API, Supported
   :widths: 50,10

    pthread_barrier_destroy(),yes
    pthread_barrier_init(),yes
    pthread_barrier_wait(),yes
    pthread_barrierattr_destroy(),yes
    pthread_barrierattr_init(),yes

.. _posix_option_group_c_lang_jump:

POSIX_C_LANG_JUMP
+++++++++++++++++

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
+++++++++++++++++

The ``POSIX_C_LANG_MATH`` Option Group is included in the ISO C standard.

.. note::
   When using Newlib, Picolibc, or other C libraries conforming to the ISO C Standard, the
   ``POSIX_C_LANG_MATH`` Option Group is considered supported.

Please refer to `Subprofiling Considerations`_ for details on the ``POSIX_C_LANG_MATH`` Option
Group.

.. _posix_option_group_c_lang_support:

POSIX_C_LANG_SUPPORT
++++++++++++++++++++

The POSIX_C_LANG_SUPPORT option group contains the general ISO C Library.

.. note::
   When using Newlib, Picolibc, or other C libraries conforming to the ISO C Standard, the entire
   ``POSIX_C_LANG_SUPPORT`` Option Group is considered supported.

Please refer to `Subprofiling Considerations`_ for details on the ``POSIX_C_LANG_SUPPORT`` Option
Group.

For more information on developing Zephyr applications in the C programming language, please refer
to :ref:`details<language_support>`.

.. _posix_option_group_c_lang_support_r:

POSIX_C_LANG_SUPPORT_R
++++++++++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_C_LANG_SUPPORT_R`.

.. csv-table:: POSIX_C_LANG_SUPPORT_R
   :header: API, Supported
   :widths: 50,10

    asctime_r(),yes
    ctime_r(),yes
    gmtime_r(),yes
    localtime_r(),yes
    rand_r(),yes
    strerror_r(),yes
    strtok_r(),yes

.. _posix_option_group_c_lib_ext:

POSIX_C_LIB_EXT
+++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_C_LIB_EXT`.

.. csv-table:: POSIX_C_LIB_EXT
   :header: API, Supported
   :widths: 50,10

    fnmatch(), yes
    getopt(), yes
    getsubopt(),
    optarg, yes
    opterr, yes
    optind, yes
    optopt, yes
    stpcpy(),
    stpncpy(),
    strcasecmp(),
    strdup(),
    strfmon(),
    strncasecmp(), yes
    strndup(),
    strnlen(), yes

.. _posix_option_group_clock_selection:

POSIX_CLOCK_SELECTION
+++++++++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_CLOCK_SELECTION`.

.. csv-table:: POSIX_CLOCK_SELECTION
   :header: API, Supported
   :widths: 50,10

    pthread_condattr_getclock(),yes
    pthread_condattr_setclock(),yes
    clock_nanosleep(),yes

.. _posix_option_group_device_io:

POSIX_DEVICE_IO
+++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_DEVICE_IO`.

.. note::
   When using Newlib, Picolibc, or other C libraries conforming to the ISO C Standard, the
   C89 components of the ``POSIX_DEVICE_IO`` Option Group are considered supported.

.. csv-table:: POSIX_DEVICE_IO
   :header: API, Supported
   :widths: 50,10

    FD_CLR(),yes
    FD_ISSET(),yes
    FD_SET(),yes
    FD_ZERO(),yes
    clearerr(),yes
    close(),yes
    fclose(),yes
    fdopen(),yes
    feof(),yes
    ferror(),yes
    fflush(),yes
    fgetc(),yes
    fgets(),yes
    fileno(),yes
    fopen(),yes
    fprintf(),yes
    fputc(),yes
    fputs(),yes
    fread(),yes
    freopen(),yes
    fscanf(),yes
    fwrite(),yes
    getc(),yes
    getchar(),yes
    gets(),yes
    open(),yes
    perror(),yes
    poll(),yes
    printf(),yes
    pread(),yes
    pselect(),yes
    putc(),yes
    putchar(),yes
    puts(),yes
    pwrite(),yes
    read(),yes
    scanf(),yes
    select(),yes
    setbuf(),yes
    setvbuf(),yes
    stderr,yes
    stdin,yes
    stdout,yes
    ungetc(),yes
    vfprintf(),yes
    vfscanf(),yes
    vprintf(),yes
    vscanf(),yes
    write(),yes

.. _posix_option_group_fd_mgmt:

POSIX_FD_MGMT
+++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_FD_MGMT`.

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
++++++++++++++++++

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

.. _posix_option_group_file_system:

POSIX_FILE_SYSTEM
+++++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_FILE_SYSTEM`.

.. csv-table:: POSIX_FILE_SYSTEM
   :header: API, Supported
   :widths: 50,10

    access(),
    chdir(),
    closedir(), yes
    creat(),
    fchdir(),
    fpathconf(),
    fstat(), yes
    fstatvfs(),
    getcwd(),
    link(),
    mkdir(), yes
    mkstemp(),
    opendir(), yes
    pathconf(),
    readdir(), yes
    remove(), yes
    rename(), yes
    rewinddir(),
    rmdir(), yes
    stat(), yes
    statvfs(),
    tmpfile(),
    tmpnam(),
    truncate(),
    unlink(), yes
    utime(),

.. _posix_option_group_file_system_r:

POSIX_FILE_SYSTEM_R
+++++++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_FILE_SYSTEM_R`.

.. csv-table:: POSIX_FILE_SYSTEM_R
   :header: API, Supported
   :widths: 50,10

    readdir_r(), yes

.. _posix_option_group_mapped_files:

POSIX_MAPPED_FILES
++++++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_MAPPED_FILES`.

.. csv-table:: POSIX_MAPPED_FILES
   :header: API, Supported
   :widths: 50,10

    mmap(),yes
    msync(),yes
    munmap(),yes

.. _posix_option_group_memory_protection:

POSIX_MEMORY_PROTECTION
+++++++++++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_MEMORY_PROTECTION`.

.. csv-table:: POSIX_MEMORY_PROTECTION
   :header: API, Supported
   :widths: 50,10

    mprotect(), yes :ref:`†<posix_undefined_behaviour>`

.. _posix_option_group_multi_process:

POSIX_MULTI_PROCESS
+++++++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_MULTI_PROCESS`.

.. csv-table:: POSIX_MULTI_PROCESS
   :header: API, Supported
   :widths: 50,10

    _Exit(), yes
    _exit(), yes
    assert(), yes
    atexit(),:ref:`†<posix_undefined_behaviour>`
    clock(),
    execl(),:ref:`†<posix_undefined_behaviour>`
    execle(),:ref:`†<posix_undefined_behaviour>`
    execlp(),:ref:`†<posix_undefined_behaviour>`
    execv(),:ref:`†<posix_undefined_behaviour>`
    execve(),:ref:`†<posix_undefined_behaviour>`
    execvp(),:ref:`†<posix_undefined_behaviour>`
    exit(), yes
    fork(),:ref:`†<posix_undefined_behaviour>`
    getpgrp(),:ref:`†<posix_undefined_behaviour>`
    getpgid(),:ref:`†<posix_undefined_behaviour>`
    getpid(), yes :ref:`†<posix_undefined_behaviour>`
    getppid(),:ref:`†<posix_undefined_behaviour>`
    getsid(),:ref:`†<posix_undefined_behaviour>`
    setsid(),:ref:`†<posix_undefined_behaviour>`
    sleep(),yes
    times(),
    wait(),:ref:`†<posix_undefined_behaviour>`
    waitid(),:ref:`†<posix_undefined_behaviour>`
    waitpid(),:ref:`†<posix_undefined_behaviour>`

.. _posix_option_group_networking:

POSIX_NETWORKING
++++++++++++++++

The function ``sockatmark()`` is not yet supported and is expected to fail setting ``errno``
to ``ENOSYS`` :ref:`†<posix_undefined_behaviour>`.

Enable this option group with :kconfig:option:`CONFIG_POSIX_NETWORKING`.

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
    sockatmark(),yes :ref:`†<posix_undefined_behaviour>`
    socketpair(),yes

.. _posix_option_group_pipe:

POSIX_PIPE
++++++++++

.. csv-table:: POSIX_PIPE
   :header: API, Supported
   :widths: 50,10

    pipe(),

.. _posix_option_group_realtime_signals:

POSIX_REALTIME_SIGNALS
++++++++++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_REALTIME_SIGNALS`.

.. csv-table:: POSIX_REALTIME_SIGNALS
   :header: API, Supported
   :widths: 50,10

    sigqueue(),
    sigtimedwait(),
    sigwaitinfo(),

.. _posix_option_group_semaphores:

POSIX_SEMAPHORES
++++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_SEMAPHORES`.

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

.. _posix_option_group_signal_jump:

POSIX_SIGNAL_JUMP
+++++++++++++++++

.. csv-table:: POSIX_SIGNAL_JUMP
   :header: API, Supported
   :widths: 50,10

    siglongjmp(),
    sigsetjmp(),

.. _posix_option_group_signals:

POSIX_SIGNALS
+++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_SIGNALS`.

.. note::
   As processes are not yet supported in Zephyr, the ISO C functions ``abort()``, ``signal()``,
   and ``raise()``, as well as the other POSIX functions listed below, may exhibit undefined
   behaviour. The POSIX functions ``kill()``, ``pause()``, ``sigaction()``, ``sigpending()``,
   ``sigsuspend()``, and ``sigwait()`` are implemented to ensure that conformant applications can
   link, but they are expected to fail, setting errno to ``ENOSYS``
   :ref:`†<posix_undefined_behaviour>`.

.. csv-table:: POSIX_SIGNALS
   :header: API, Supported
   :widths: 50,10

    abort(),yes :ref:`†<posix_undefined_behaviour>`
    alarm(),yes :ref:`†<posix_undefined_behaviour>`
    kill(),yes :ref:`†<posix_undefined_behaviour>`
    pause(),yes :ref:`†<posix_undefined_behaviour>`
    raise(),yes :ref:`†<posix_undefined_behaviour>`
    sigaction(),yes :ref:`†<posix_undefined_behaviour>`
    sigaddset(),yes
    sigdelset(),yes
    sigemptyset(),yes
    sigfillset(),yes
    sigismember(),yes
    signal(),yes :ref:`†<posix_undefined_behaviour>`
    sigpending(),yes :ref:`†<posix_undefined_behaviour>`
    sigprocmask(),yes
    sigsuspend(),yes :ref:`†<posix_undefined_behaviour>`
    sigwait(),yes :ref:`†<posix_undefined_behaviour>`
    strsignal(),yes

.. _posix_option_group_single_process:

POSIX_SINGLE_PROCESS
++++++++++++++++++++

The POSIX_SINGLE_PROCESS option group contains services for single
process applications.

Enable this option group with :kconfig:option:`CONFIG_POSIX_SINGLE_PROCESS`.

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

.. _posix_option_group_spin_locks:

POSIX_SPIN_LOCKS
++++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_SPIN_LOCKS`.

.. csv-table:: POSIX_SPIN_LOCKS
   :header: API, Supported
   :widths: 50,10

    pthread_spin_destroy(),yes
    pthread_spin_init(),yes
    pthread_spin_lock(),yes
    pthread_spin_trylock(),yes
    pthread_spin_unlock(),yes

.. _posix_option_group_threads_base:

POSIX_THREADS_BASE
++++++++++++++++++

The basic assumption in this profile is that the system
consists of a single (implicit) process with multiple threads. Therefore, the
standard requires all basic thread services, except those related to
multiple processes.

Enable this option group with :kconfig:option:`CONFIG_POSIX_THREADS`.

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
+++++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_THREADS_EXT`.

.. csv-table:: POSIX_THREADS_EXT
   :header: API, Supported
   :widths: 50,10

    pthread_attr_getguardsize(),yes
    pthread_attr_setguardsize(),yes
    pthread_mutexattr_gettype(),yes
    pthread_mutexattr_settype(),yes

.. _posix_option_group_timers:

POSIX_TIMERS
++++++++++++

Enable this option group with :kconfig:option:`CONFIG_POSIX_TIMERS`.

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

.. _posix_option_group_xsi_realtime:

XSI_REALTIME
++++++++++++

The ``XSI_REALTIME`` option group indicates that the :ref:`_POSIX_FSYNC<posix_option_fsync>`,
:ref:`_POSIX_MEMLOCK<posix_option_memlock>`,
:ref:`_POSIX_MEMLOCK_RANGE<posix_option_memlock_range>`,
:ref:`_POSIX_MESSAGE_PASSING<posix_option_message_passing>`,
:ref:`_POSIX_PRIORITY_SCHEDULING<posix_option_priority_scheduling>`,
:ref:`_POSIX_SHARED_MEMORY_OBJECTS<posix_option_shared_memory_objects>`, and
:ref:`_POSIX_SYNCHRONIZED_IO<posix_option_synchronized_io>` options are enabled.

Enable this option group with :kconfig:option:`CONFIG_XSI_REALTIME`.

When this option group is enabled, the ``_XOPEN_REALTIME`` feature test macro will be defined to a
value other than -1.

.. _posix_option_group_xsi_single_process:

XSI_SINGLE_PROCESS
++++++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_XSI_SINGLE_PROCESS`.

.. csv-table:: XSI_SINGLE_PROCESS
   :header: API, Supported
   :widths: 50,10

    gethostid(),yes
    gettimeofday(),yes
    putenv(),yes

.. _posix_option_group_xsi_system_logging:

XSI_SYSTEM_LOGGING
++++++++++++++++++

Enable this option group with :kconfig:option:`CONFIG_XSI_SYSTEM_LOGGING`.

.. csv-table:: XSI_SYSTEM_LOGGING
   :header: API, Supported
   :widths: 50,10

    closelog(),yes
    openlog(),yes
    setlogmask(),yes
    syslog(),yes

.. _posix_option_group_xsi_threads_ext:

XSI_THREADS_EXT
+++++++++++++++

The XSI_THREADS_EXT option group is required because it provides
functions to control a thread's stack. This is considered useful for any
real-time application.

Enable this option group with :kconfig:option:`CONFIG_XSI_THREADS_EXT`.

.. csv-table:: XSI_THREADS_EXT
   :header: API, Supported
   :widths: 50,10

    pthread_attr_getstack(),yes
    pthread_attr_setstack(),yes
    pthread_getconcurrency(),yes
    pthread_setconcurrency(),yes

.. _posix_options:

POSIX Options
=============

.. _posix_option_asynchronous_io:

_POSIX_ASYNCHRONOUS_IO
++++++++++++++++++++++

Functions part of the ``_POSIX_ASYNCHRONOUS_IO`` Option are not implemented in Zephyr but are
provided so that conformant applications can still link. These functions will fail, setting
``errno`` to ``ENOSYS``:ref:`†<posix_undefined_behaviour>`.

Enable this option with :kconfig:option:`CONFIG_POSIX_ASYNCHRONOUS_IO`.

.. csv-table:: _POSIX_ASYNCHRONOUS_IO
   :header: API, Supported
   :widths: 50,10

    aio_cancel(),yes :ref:`†<posix_undefined_behaviour>`
    aio_error(),yes :ref:`†<posix_undefined_behaviour>`
    aio_fsync(),yes :ref:`†<posix_undefined_behaviour>`
    aio_read(),yes :ref:`†<posix_undefined_behaviour>`
    aio_return(),yes :ref:`†<posix_undefined_behaviour>`
    aio_suspend(),yes :ref:`†<posix_undefined_behaviour>`
    aio_write(),yes :ref:`†<posix_undefined_behaviour>`
    lio_listio(),yes :ref:`†<posix_undefined_behaviour>`

.. _posix_option_cputime:

_POSIX_CPUTIME
++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_CPUTIME`.

.. csv-table:: _POSIX_CPUTIME
   :header: API, Supported
   :widths: 50,10

    CLOCK_PROCESS_CPUTIME_ID,yes

.. _posix_option_fsync:

_POSIX_FSYNC
++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_FSYNC`.

.. csv-table:: _POSIX_FSYNC
   :header: API, Supported
   :widths: 50,10

    fsync(),yes

.. _posix_option_ipv6:

_POSIX_IPV6
+++++++++++

Internet Protocol Version 6 is supported.

For more information, please refer to :ref:`Networking <networking>`.

Enable this option with :kconfig:option:`CONFIG_POSIX_IPV6`.

.. _posix_option_memlock:

_POSIX_MEMLOCK
++++++++++++++

Zephyr's :ref:`Demand Paging API <memory_management_api_demand_paging>` does not yet support
pinning or unpinning all virtual memory regions. The functions below are expected to fail and
set ``errno`` to ``ENOSYS`` :ref:`†<posix_undefined_behaviour>`.

Enable this option with :kconfig:option:`CONFIG_POSIX_MEMLOCK`.

.. csv-table:: _POSIX_MEMLOCK
   :header: API, Supported
   :widths: 50,10

    mlockall(), yes
    munlockall(), yes

.. _posix_option_memlock_range:

_POSIX_MEMLOCK_RANGE
++++++++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_MEMLOCK_RANGE`.

.. csv-table:: _POSIX_MEMLOCK_RANGE
   :header: API, Supported
   :widths: 50,10

    mlock(), yes
    munlock(), yes

.. _posix_option_message_passing:

_POSIX_MESSAGE_PASSING
++++++++++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_MESSAGE_PASSING`.

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

Enable this option with :kconfig:option:`CONFIG_POSIX_MONOTONIC_CLOCK`.

.. csv-table:: _POSIX_MONOTONIC_CLOCK
   :header: API, Supported
   :widths: 50,10

    CLOCK_MONOTONIC,yes

.. _posix_option_priority_scheduling:

_POSIX_PRIORITY_SCHEDULING
++++++++++++++++++++++++++

As processes are not yet supported in Zephyr, the functions ``sched_rr_get_interval()``,
``sched_setparam()``, and ``sched_setscheduler()`` are expected to fail setting ``errno``
to ``ENOSYS``:ref:`†<posix_undefined_behaviour>`.

Enable this option with :kconfig:option:`CONFIG_POSIX_PRIORITY_SCHEDULING`.

.. csv-table:: _POSIX_PRIORITY_SCHEDULING
   :header: API, Supported
   :widths: 50,10

    sched_get_priority_max(),yes
    sched_get_priority_min(),yes
    sched_getparam(),yes
    sched_getscheduler(),yes
    sched_rr_get_interval(),yes :ref:`†<posix_undefined_behaviour>`
    sched_setparam(),yes :ref:`†<posix_undefined_behaviour>`
    sched_setscheduler(),yes :ref:`†<posix_undefined_behaviour>`
    sched_yield(),yes

.. _posix_option_raw_sockets:

_POSIX_RAW_SOCKETS
++++++++++++++++++

Raw sockets are supported.

For more information, please refer to :kconfig:option:`CONFIG_NET_SOCKETS_PACKET`.

Enable this option with :kconfig:option:`CONFIG_POSIX_RAW_SOCKETS`.

.. _posix_option_reader_writer_locks:

_POSIX_READER_WRITER_LOCKS
++++++++++++++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_READER_WRITER_LOCKS`.

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

..
   this link is "deprecated" - mainly left here so that older links still work
.. _posix_shared_memory_objects:

.. _posix_option_shared_memory_objects:

_POSIX_SHARED_MEMORY_OBJECTS
++++++++++++++++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_SHARED_MEMORY_OBJECTS`.

.. csv-table:: _POSIX_SHARED_MEMORY_OBJECTS
   :header: API, Supported
   :widths: 50,10

    mmap(), yes
    munmap(), yes
    shm_open(), yes
    shm_unlink(), yes

.. _posix_option_synchronized_io:

_POSIX_SYNCHRONIZED_IO
++++++++++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_SYNCHRONIZED_IO`.

.. csv-table:: _POSIX_SYNCHRONIZED_IO
   :header: API, Supported
   :widths: 50,10

    fdatasync(),yes
    fsync(),yes
    msync(),yes

.. _posix_option_thread_attr_stackaddr:

_POSIX_THREAD_ATTR_STACKADDR
++++++++++++++++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_THREAD_ATTR_STACKADDR`.

.. csv-table:: _POSIX_THREAD_ATTR_STACKADDR
   :header: API, Supported
   :widths: 50,10

    pthread_attr_getstackaddr(),yes
    pthread_attr_setstackaddr(),yes

.. _posix_option_thread_attr_stacksize:

_POSIX_THREAD_ATTR_STACKSIZE
++++++++++++++++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_THREAD_ATTR_STACKSIZE`.

.. csv-table:: _POSIX_THREAD_ATTR_STACKSIZE
   :header: API, Supported
   :widths: 50,10

    pthread_attr_getstacksize(),yes
    pthread_attr_setstacksize(),yes

.. _posix_option_thread_cputime:

_POSIX_THREAD_CPUTIME
+++++++++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_THREAD_CPUTIME`.

.. csv-table:: _POSIX_THREAD_CPUTIME
   :header: API, Supported
   :widths: 50,10

    CLOCK_THREAD_CPUTIME_ID,yes
    pthread_getcpuclockid(),yes

.. _posix_option_thread_prio_inherit:

_POSIX_THREAD_PRIO_INHERIT
++++++++++++++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_THREAD_PRIO_INHERIT`.

.. csv-table:: _POSIX_THREAD_PRIO_INHERIT
   :header: API, Supported
   :widths: 50,10

    pthread_mutexattr_getprotocol(),yes
    pthread_mutexattr_setprotocol(),yes

.. _posix_option_thread_prio_protect:

_POSIX_THREAD_PRIO_PROTECT
++++++++++++++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_THREAD_PRIO_PROTECT`.

.. csv-table:: _POSIX_THREAD_PRIO_PROTECT
   :header: API, Supported
   :widths: 50,10

    pthread_mutex_getprioceiling(),yes
    pthread_mutex_setprioceiling(),yes
    pthread_mutexattr_getprioceiling(),yes
    pthread_mutexattr_getprotocol(),yes
    pthread_mutexattr_setprioceiling(),yes
    pthread_mutexattr_setprotocol(),yes

.. _posix_option_thread_priority_scheduling:

_POSIX_THREAD_PRIORITY_SCHEDULING
+++++++++++++++++++++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_THREAD_PRIORITY_SCHEDULING`.

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

.. _posix_option_thread_safe_functions:

_POSIX_THREAD_SAFE_FUNCTIONS
++++++++++++++++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_THREAD_SAFE_FUNCTIONS`.

.. csv-table:: _POSIX_THREAD_SAFE_FUNCTIONS
    :header: API, Supported
    :widths: 50,10

    asctime_r(), yes
    ctime_r(), yes (UTC timezone only)
    flockfile(),
    ftrylockfile(),
    funlockfile(),
    getc_unlocked(),
    getchar_unlocked(),
    getgrgid_r(),yes :ref:`†<posix_undefined_behaviour>`
    getgrnam_r(),yes :ref:`†<posix_undefined_behaviour>`
    getpwnam_r(),yes :ref:`†<posix_undefined_behaviour>`
    getpwuid_r(),yes :ref:`†<posix_undefined_behaviour>`
    gmtime_r(), yes
    localtime_r(), yes (UTC timezone only)
    putc_unlocked(),
    putchar_unlocked(),
    rand_r(), yes
    readdir_r(), yes
    strerror_r(), yes
    strtok_r(), yes

.. _posix_option_timeouts:

_POSIX_TIMEOUTS
+++++++++++++++

Enable this option with :kconfig:option:`CONFIG_POSIX_TIMEOUTS`.

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

With the exception of ``ioctl()``, functions in the ``_XOPEN_STREAMS`` option group are not
implemented in Zephyr but are provided so that conformant applications can still link.
Unimplemented functions in this option group will fail, setting ``errno`` to ``ENOSYS``
:ref:`†<posix_undefined_behaviour>`.

Enable this option with :kconfig:option:`CONFIG_XOPEN_STREAMS`.

.. csv-table:: _XOPEN_STREAMS
   :header: API, Supported
   :widths: 50,10

    fattach(), yes :ref:`†<posix_undefined_behaviour>`
    fdetach(), yes :ref:`†<posix_undefined_behaviour>`
    getmsg(), yes :ref:`†<posix_undefined_behaviour>`
    getpmsg(), yes :ref:`†<posix_undefined_behaviour>`
    ioctl(), yes
    isastream(), yes :ref:`†<posix_undefined_behaviour>`
    putmsg(), yes :ref:`†<posix_undefined_behaviour>`
    putpmsg(), yes :ref:`†<posix_undefined_behaviour>`

.. _Subprofiling Considerations:
    https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_subprofiles.html
