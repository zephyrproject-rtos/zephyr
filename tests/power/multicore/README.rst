Multicore Test App
##################

These applications aim to provide an easy way to test the PM multicore support
from Quark SE. It implements two common flows: 1) LMT core is idle while ARC
core is busy so the system is not put on DEEP SLEEP mode, and 2) LMT core is
idle when ARC core is also idle so the system enters in DEEP SLEEP mode.

These applications implement a master/slave approach where LMT application
plays the master role while ARC application plays the slave. The master is the
one which controls the wakeup device (in this example it is the RTC) and
actually puts the system in deep sleep mode.

To keep the synchronization logic simple and demonstrate the multi-core
coordination, we do not handle any events in ARC other than the notification
IPM from LMT. Handling events in ARC itself may need more complex communication
and synchronization logic between the applications of the the 2 cores. For
example, such an implementation should be able to handle the corner case of
ARC getting woken up by some event when LMT is in the process of putting the
SOC in deep sleep.

In the following section the working mechanism from both ARC and LMT
applications are described, and building and wiring instructions are
provided.

test/power/multicore/arc
************************

The ARC application is very simple, it keeps the system bouncing between 'busy'
and 'idle' states. The application keeps the system busy for 10 seconds and
then idle until woken up by IPM event from LMT. When system goes into idle,
the application puts the system in SYS_POWER_STATE_DEEP_SLEEP_2 state which
allows LMT core to put the system in sleep mode.

The application uses UART_0 as console output device so, in order to be able
to see ARC output messages, make sure you have attached a serial cable to
UART_0. In 'quark_se_c1000_ss_devboard', UART_0 pins are in J14 header. The
table below shows the wiring instructions.

+---------+------------------+
| J14 PIN | SERIAL CABLE PIN |
+=========+==================+
|   3     |       RXD        |
+---------+------------------+
|   5     |       TXD        |
+---------+------------------+
|   11    |       GND        |
+---------+------------------+

If your wiring is correct, you should see the following output on console:

::

    ARC: Quark SE PM Multicore Demo
    ARC: busy
    ARC: idle
    ARC: busy
    ARC: idle
    ARC: busy
    ARC: idle
    ARC: busy
    ...

To build the ARC application, run the following commands:

::

    $ cd tests/power/multicore/arc/
    $ make

test/power/multicore/lmt
************************

The LMT application is very simple and also keeps the system bouncing between
'busy' and 'idle'. When the system goes into idle, the application tries to
put the system in DEEP_SLEEP state. If ARC core is busy, it fails. If ARC core
is idle, it succeeds.

When 'TEST_CASE=sleep-success', the application will be busy for 15 seconds
and idle for 5. This means that ARC core will be idle when LMT core tries to
enter in DEEP_SLEEP, and it will succeed. In this case, the output on your
console should look like this:

::

    LMT: Quark SE PM Multicore Demo
    LMT: busy
    LMT: idle
    LMT: Try to put the system in SYS_POWER_STATE_DEEP_SLEEP_2 state
    LMT: Succeed.
    LMT: busy
    LMT: idle
    LMT: Try to put the system in SYS_POWER_STATE_DEEP_SLEEP_2 state
    LMT: Succeed.
    LMT: busy
    LMT: idle
    LMT: Try to put the system in SYS_POWER_STATE_DEEP_SLEEP_2 state
    LMT: Succeed.
    ...

To build the LMT application which tests the "success" path, run the following
commands:

::

    $ cd tests/power/multicore/lmt TEST_CASE=sleep-success
    $ make

When 'TEST_CASE=sleep-fail', application is busy for 5 seconds and idle for 15
seconds. This means that ARC core will be busy when LMT core tries to enter in
DEEP_SLEEP, and it will fail. In this case the output on your console should
look like this:

::

    LMT: Quark SE PM Multicore Demo
    LMT: busy
    LMT: idle
    LMT: Try to put the system in SYS_POWER_STATE_DEEP_SLEEP_2 state
    LMT: Failed. ARC is busy.
    LMT: busy
    LMT: idle
    LMT: Try to put the system in SYS_POWER_STATE_DEEP_SLEEP_2 state
    LMT: Failed. ARC is busy.
    LMT: busy
    LMT: idle
    LMT: Try to put the system in SYS_POWER_STATE_DEEP_SLEEP_2 state
    LMT: Failed. ARC is busy.
    ...

To build the LMT application which tests the "failure" path, run the following
commands:

::

    $ cd tests/power/multicore/lmt
    $ make TEST_CASE=sleep-fail

The application uses UART_1 device as console output device, which is the
default console device.
