.. _ctf:

Common Trace Format for Zephyr
##############################

Common Trace Format, CTF, is an open format and language to describe trace
formats in.  This enables tool reuse, of which line-textual (babeltrace)
and graphical (TraceCompass) variants already exist.

CTF should look familiar to C programmers but adds stronger typing.
See `CTF - A  Flexible, High-performance Binary Trace Format
<http://diamon.org/ctf/>`_.

Every system has application-specific events to trace out.  Historically,
that has implied:

1. Determining the application-specific payload,
2. Choosing suitable serialization-format,
3. Writing the on-target serialization code,
4. Deciding on and writing the IO transport mechanics,
5. Writing the PC-side deserializer/parser,
6. Writing custom ad-hoc tools for filtering and presentation.

CTF allows us to formally describe #1 and #2, which enables common
infrastructure for #5 and #6.  This leaves #3 serialization code and #4
IO mechanics up to a custom implementation.

This CTF debug module aims at providing a common #1 and #2 for Zephyr
("middle"), while providing a lean & generic interface for IO ("bottom").
Currently, only one CTF bottom-layer exists, POSIX ``fwrite``, but many others
are possible:

- Async UART
- Async DMA
- Sync GPIO
- ... and many more.

In fact, IO varies greatly from system to system.  Therefore, it is
instructive to create a taxonomy for IO types when we must ensure the
interface between CTF-middle and CTF-bottom is generic and efficient
enough to model these. See the *IO taxonomy* section below.


A generic interface
-------------------

In CTF, an event is serialized to a packet containing one or more fields
and as seen from *IO taxonomy* section below, a bottom layer:

- may perform action at transaction-start (e.g. mutex-lock),
- may process each field in some way (e.g. sync-push emit, concat, enqueue to thread-bound FIFO),
- may perform action at transaction-stop (e.g. mutex-release, emit of concat buffer).

This is possible when the bottom-layer implements the following macros:

- ``CTF_BOTTOM_LOCK``:   No-op or how to lock the IO transaction.
- ``CTF_BOTTOM_UNLOCK``: No-op or how to release the IO transaction.
- ``CTF_BOTTOM_FIELDS``: Var-args of fields. May process each field with ``MAP``.
- ``CTF_BOTTOM_TIMESTAMPED_INTERNALLY``: Tells where timestamping is done.

These macros along with inline functions of the middle-layer can yield
very low-overhead tracing infrastructure.


CTF middle-layer example
------------------------

The CTF_EVENT macro will serialize each argument to a field::

  /* Example for illustration */
  static inline void ctf_middle_foo(u32_t thread_id, ctf_bounded_string_t name)
  {
    CTF_EVENT(
      CTF_LITERAL(u8_t, 42),
      thread_id,
      name,
      "hello, I was emitted from function: ",
      __func__  /* __func__ is standard since C99 */
    );
  }

How to serialize and emit fields as well as handling alignment, can be done
internally and statically at compile-time in the bottom-layer.


How to activate?
----------------

Make sure ``CONFIG_CTF=y`` and ``CONFIG_CTF_BOTTOM_POSIX=y`` is set.



What is TraceCompass?
---------------------

TraceCompass is an open source tool that visualizes CTF events such
as thread scheduling and interrupts, and is helpful to find unintended
interactions and resource conflicts on complex systems.

See also the presentation by Ericsson,
`Advanced Trouble-shooting Of Real-time Systems
<https://wiki.eclipse.org/images/0/0e/TechTalkOnlineDemoFeb2017_v1.pdf>`_.


Future LTTng inspiration
------------------------

Currently, the middle-layer provided here is quite simple and bare-bones,
and needlessly copied from Zephyr's Segger SystemView debug module.

For an OS like Zephyr, it would make sense to draw inspiration from
Linux' LTTng and change the middle-layer to serialize to the same format.
Doing this would enable direct reuse of TraceCompass' canned analyses
for Linux.  Alternatively, LTTng-analyses in TraceCompass could be
customized to Zephyr.  It is ongoing work to enable TraceCompass
visibility of Zephyr in a target-agnostic and open source way.


IO taxonomy
-----------

  - Atomic Push/Produce/Write/Enqueue:

    - synchronous:  means data-transmission has completed with the return of the call.

    - asynchronous: means data-transmission is pending or ongoing with the return of the call.
            Usually, interrupts/callbacks/signals or polling is used to determine completion.

    - buffered:     means data-transmissions are copied and grouped together to form a larger ones.
            Usually for amortizing overhead (burst dequeue) or jitter-mitigation (steady dequeue).

    Examples:
      - sync  unbuffered
          E.g. PIO via GPIOs having steady stream, no extra FIFO memory needed.
          Low jitter but may be less efficient (cant amortize the overhead of writing).

      - sync  buffered
          E.g. ``fwrite()`` or enqueuing into FIFO.
          Blockingly burst the FIFO when its buffer-waterlevel exceeds threshold.
          Jitter due to bursts may lead to missed deadlines.

      - async unbuffered
          E.g. DMA, or zero-copying in shared memory.
          Be careful of data hazards, race conditions, etc!

      - async buffered
          E.g. enqueuing into FIFO.



  - Atomic Pull/Consume/Read/Dequeue:

    - synchronous:  means data-reception has completed with the return of the call.

    - asynchronous: means data-reception is pending or ongoing with the return of the call.
          Usually, interrupts/callbacks/signals or polling is used to determine completion.

    - buffered:     means data is copied-in in larger chunks than request-size.
          Usually for amortizing wait-time.

    Examples:
      - sync  unbuffered
          E.g. Blocking read-call, ``fread()`` or SPI-read, zero-copying in shared memory.

      - sync  buffered
          E.g. Blocking read-call with caching applied.
          Makes sense if read pattern exhibits spatial locality.

      - async unbuffered
          E.g. zero-copying in shared memory.
          Be careful of data hazards, race conditions, etc!

      - async buffered
          E.g. ``aio_read()`` or DMA.



  Unfortunately, IO may not be atomic and may, therefore, require locking.
  Locking may be not be needed if multiple independent channels are available.

    - System has non-atomic write and one shared channel
          E.g. UART. Locking required.

          ``lock(); emit(a); emit(b); emit(c); release();``

    - System has non-atomic write but many channels
          E.g. Multi-UART. Lock-free if the bottom-layer maps each Zephyr
          thread+ISR to its own channel, thus alleviating races as each
          thread is sequentially consistent with itself.

          ``emit(a,thread_id); emit(b,thread_id); emit(c,thread_id);``

    - System has atomic write     but one shared channel
          E.g. ``native_posix`` or board with DMA. May or may not need locking.

          ``emit(a ## b ## c); /* Concat to buffer */``

          ``lock(); emit(a); emit(b); emit(c); release(); /* No extra mem */``

    - System has atomic write     and many channels
          E.g. native_posix or board with multi-channel DMA. Lock-free.

          ``emit(a ## b ## c, thread_id);``

