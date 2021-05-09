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
Running test suite test_obj_tracing
===================================================================
starting test - test_tracing
SEMAPHORE REF: 0x004002d0
SEMAPHORE REF: 0x004002bc
SEMAPHORE REF: 0x004002a8
SEMAPHORE REF: 0x00400294
SEMAPHORE REF: 0x00400280
SEMAPHORE REF: 0x0042402c
SEMAPHORE REF: 0x00424250
SEMAPHORE QUANTITY: 6
COOP: 0x00400040 OPTIONS: 0x00, STATE: 0x00
COOP: 0x00400200 OPTIONS: 0x00, STATE: 0x02
COOP: 0x004001a8 OPTIONS: 0x00, STATE: 0x02
COOP: 0x00400150 OPTIONS: 0x00, STATE: 0x00
COOP: 0x004000f8 OPTIONS: 0x00, STATE: 0x02
COOP: 0x004000a0 OPTIONS: 0x00, STATE: 0x00
PREMPT: 0x00401254 OPTIONS: 0x00, STATE: 0x02
COOP: 0x00401020 OPTIONS: 0x01, STATE: 0x00
COOP: 0x00401080 OPTIONS: 0x01, STATE: 0x00
THREAD QUANTITY: 9
PASS - test_tracing.
===================================================================
===================================================================
PROJECT EXECUTION SUCCESSFUL
