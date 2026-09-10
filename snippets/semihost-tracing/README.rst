.. _snippet-semihost-tracing:

Semihost Tracing Snippet (semihost-tracing)
###########################################

.. code-block:: console

   west build -S semihost-tracing [...]

Overview
********

This snippet enables CTF Tracing using semihosting on a target that supports it.

Using Qemu, you can run the example application and generate a CTF trace file
using semihosting. The trace file can then be viewed using `Babeltrace`_.

.. code-block:: console

   (gdb) dump binary memory ram_tracing.bin ram_tracing ram_tracing+8192

Using `Babeltrace`_, you can view the binary file as a CTF trace:

.. code-block:: console

   [19:00:00.023530800] (+?.?????????) thread_create: { thread_id = 2147621216, name = "unknown" }
   [19:00:00.023567100] (+0.000036300) thread_info: { thread_id = 2147621216, name = "unknown", stack_base = 2147705856, stack_size = 3072 }
   [19:00:00.023673700] (+0.000106600) thread_name_set: { thread_id = 2147621216, name = "test_abort_from_isr" }
   [19:00:00.023731100] (+0.000057400) thread_wakeup: { thread_id = 2147621216, name = "test_abort_from_isr" }
   [19:00:00.023827500] (+0.000096400) thread_switched_out: { thread_id = 2147621968, name = "main" }
   [19:00:00.023904500] (+0.000077000) thread_switched_in: { thread_id = 2147621216, name = "test_abort_from_isr" }
   [19:00:00.024807600] (+0.000903100) thread_create: { thread_id = 2147619320, name = "unknown" }
   [19:00:00.024843800] (+0.000036200) thread_info: { thread_id = 2147619320, name = "unknown", stack_base = 2147693568, stack_size = 3072 }
   [19:00:00.024898700] (+0.000054900) thread_wakeup: { thread_id = 2147619320, name = "unknown" }
   [19:00:00.025020000] (+0.000121300) thread_switched_out: { thread_id = 2147621216, name = "test_abort_from_isr" }
   [19:00:00.025130700] (+0.000110700) thread_switched_in: { thread_id = 2147621968, name = "main" }
   [19:00:00.025249900] (+0.000119200) thread_switched_out: { thread_id = 2147621968, name = "main" }
   [19:00:00.025353500] (+0.000103600) thread_switched_in: { thread_id = 2147619320, name = "unknown" }
   [19:00:00.026049900] (+0.000696400) thread_switched_in: { thread_id = 2147621216, name = "test_abort_from_isr" }
   [19:00:00.026759200] (+0.000709300) semaphore_give_enter: { id = 2147735664 }
   [19:00:00.026773600] (+0.000014400) semaphore_give_exit: { id = 2147735664 }
   [19:00:00.027346600] (+0.000573000) thread_switched_out: { thread_id = 2147621216, name = "test_abort_from_isr" }
   [19:00:00.027457400] (+0.000110800) thread_switched_in: { thread_id = 2147621968, name = "main" }
   [19:00:00.029552800] (+0.002095400) thread_create: { thread_id = 2147621216, name = "unknown" }
   [19:00:00.029589000] (+0.000036200) thread_info: { thread_id = 2147621216, name = "unknown", stack_base = 2147705856, stack_size = 3072 }
   [19:00:00.029697400] (+0.000108400) thread_name_set: { thread_id = 2147621216, name = "test_abort_from_isr" }
   [19:00:00.029754800] (+0.000057400) thread_wakeup: { thread_id = 2147621216, name = "test_abort_from_isr" }
   [19:00:00.029851200] (+0.000096400) thread_switched_out: { thread_id = 2147621968, name = "main" }
   [19:00:00.029928200] (+0.000077000) thread_switched_in: { thread_id = 2147621216, name = "test_abort_from_isr" }
   [19:00:00.029968400] (+0.000040200) semaphore_init: { id = 2147623436, ret = 0 }
   [19:00:00.030851900] (+0.000883500) thread_create: { thread_id = 2147619320, name = "unknown" }
   [19:00:00.030888100] (+0.000036200) thread_info: { thread_id = 2147619320, name = "unknown", stack_base = 2147693568, stack_size = 3072 }
   [19:00:00.030943000] (+0.000054900) thread_wakeup: { thread_id = 2147619320, name = "unknown" }
   [19:00:00.030989600] (+0.000046600) semaphore_take_enter: { id = 2147623436, timeout = 4294957296 }
   [19:00:00.031005800] (+0.000016200) semaphore_take_blocking: { id = 2147623436, timeout = 4294957296 }
   [19:00:00.031099600] (+0.000093800) thread_switched_out: { thread_id = 2147621216, name = "test_abort_from_isr" }
   [19:00:00.031210300] (+0.000110700) thread_switched_in: { thread_id = 2147621968, name = "main" }
   [19:00:00.031329500] (+0.000119200) thread_switched_out: { thread_id = 2147621968, name = "main" }
   [19:00:00.031433100] (+0.000103600) thread_switched_in: { thread_id = 2147619320, name = "unknown" }
   [19:00:00.031469100] (+0.000036000) semaphore_give_enter: { id = 2147623436 }

References
**********

.. target-notes::

.. _Babeltrace: https://babeltrace.org/
