.. _timing_functions:

Executing Time Functions
########################

The timing functions can be used to obtain execution time of
a section of code to aid in analysis and optimization.

Please note that the timing functions may use a different timer
than the default kernel timer, where the timer being used is
specified by architecture, SoC or board configuration.

Configuration
*************

To allow using the timing functions, :kconfig:option:`CONFIG_TIMING_FUNCTIONS`
needs to be enabled.

Usage
*****

To gather timing information:

1. Call :c:func:`timing_init` to initialize the timer.

2. Call :c:func:`timing_start` to signal the start of gathering of
   timing information. This usually starts the timer.

3. Call :c:func:`timing_counter_get` to mark the start of code
   execution.

4. Call :c:func:`timing_counter_get` to mark the end of code
   execution.

5. Call :c:func:`timing_cycles_get` to get the number of timer cycles
   between start and end of code execution.

6. Call :c:func:`timing_cycles_to_ns` with total number of cycles
   to convert number of cycles to nanoseconds.

7. Repeat from step 3 to gather timing information for other
   blocks of code.

8. Call :c:func:`timing_stop` to signal the end of gathering of
   timing information. This usually stops the timer.

Example
-------

This shows an example on how to use the timing functions:

.. code-block:: c

   #include <zephyr/timing/timing.h>

   void gather_timing(void)
   {
       timing_t start_time, end_time;
       uint64_t total_cycles;
       uint64_t total_ns;

       timing_init();
       timing_start();

       start_time = timing_counter_get();

       code_execution_to_be_measured();

       end_time = timing_counter_get();

       total_cycles = timing_cycles_get(&start_time, &end_time);
       total_ns = timing_cycles_to_ns(total_cycles);

       timing_stop();
   }

API documentation
*****************

.. doxygengroup:: timing_api
.. doxygengroup:: timing_api_arch
.. doxygengroup:: timing_api_soc
.. doxygengroup:: timing_api_board
