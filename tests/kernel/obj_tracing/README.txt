Test Description
----------------

The object tracing test is a sanity test to verify that the
object tracing API remains healthy.

It uses the philsophers as an application that implements
multiple threads that are synchronized with semaphores.

The application initializes their objects and starts the philosophers'
thread interaction. A specific thread, called object monitor, accesses
the object tracing API and reports the number of expected objects.

The sanity test script expects each test to finish its execution
and then it considers the test completed. For that reason the
philosophers' threads execute a finite number of iterations. After
that the application execution ends.

Sample Output
--------------
***** BOOTING ZEPHYR OS vxxxx - BUILD: yyyyy *****
tc_start() - OBJECT TRACING TEST
SEMAPHORE REF: 0x001031f0
SEMAPHORE REF: 0x001031dc
SEMAPHORE REF: 0x001031c8
SEMAPHORE REF: 0x001031b4
SEMAPHORE REF: 0x001031a0
SEMAPHORE QUANTITY: 5
===================================================================
PASS - object_monitor.
COOP: 0x00102da0 OPTIONS: 0x00, STATE: 0x00
COOP: 0x00104204 OPTIONS: 0x00, STATE: 0x00
COOP: 0x00103e04 OPTIONS: 0x00, STATE: 0x00
COOP: 0x00103a04 OPTIONS: 0x00, STATE: 0x02
COOP: 0x00103604 OPTIONS: 0x00, STATE: 0x02
COOP: 0x00103204 OPTIONS: 0x00, STATE: 0x00
PREMPT: 0x00105340 OPTIONS: 0x00, STATE: 0x02
COOP: 0x00104e40 OPTIONS: 0x01, STATE: 0x00
THREAD QUANTITY: 8
===================================================================
PASS - test_thread_monitor.
===================================================================
PROJECT EXECUTION SUCCESSFUL
