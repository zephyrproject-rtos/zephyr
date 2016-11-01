Test Description
----------------

The object tracing test is a sanity test to verify that the
object tracing API remains healthy.

It uses the philsophers as an application that implements
multiple taks that are synchronized with mutexes.

The application initializes their objects and starts the philosophers'
task interaction. A specific task, called object monitor, accesses
the object tracing API and reports the number of expected objects.

The sanity test script expects each test to finish its execution
and then it considers the test completed. For that reason the
philosophers' threads execute a finite number of iterations. After
that the application execution ends.
