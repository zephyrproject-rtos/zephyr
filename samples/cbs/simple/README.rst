.. zephyr:code-sample:: cbs_simple
   :name: CBS simple

   A sample to understand the CBS lifecycle and main events.

Overview
********

A sample that creates one Constant Bandwidth Server (CBS) and pushes
a simple job to the server queue three times every two seconds. In
this example, the job simply prints some information in the console
and increments a counter. The main thread regularly pushes some jobs
to the CBS queue and sleeps for 2 seconds, starting over afterwards.

Event logging
=============

The logging feature is enabled by default, which allows users to see
the key events of a CBS in the console as they happen. The events are:

.. list-table:: Constant Bandwidth Server (CBS) events
   :widths: 10 90
   :header-rows: 1

   * - Event
     - Description
   * - J_PUSH
     - a job has been pushed to the CBS job queue.
   * - J_COMP
     - a job has been completed.
   * - B_COND
     - the kernel verified that the available (budget, deadline) pair yielded
       a higher bandwidth than what's configured for that CBS. This condition
       is only checked right after J_PUSH happens. When true, the deadline is
       recalculated as (t + period) and the budget is replenished.
   * - B_ROUT
     - the budget ran out mid-execution and was replenished. The deadline is
       postponed in (period) units (i.e. deadline += period).
   * - SWT_TO
     - the CBS thread entered the CPU to start or resume the execution of a job.
   * - SWT_AY
     - the CBS thread left the CPU due to preemption or completing a job execution.

The server deadline recalculates whenever the budget is restored - that is, when
either the condition is met (B_COND) or it simply runs out (B_ROUT). In the first
case, the deadline will be set as job arrival instant + CBS period, whereas in the
latter it will be simply postponed by the CBS period.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/cbs/simple
   :host-os: unix
   :board: qemu_riscv32
   :goals: run
   :compact:

To build and run on a physical target (i.e. XIAO ESP32-C3) instead,
run the following:

.. zephyr-app-commands::
   :zephyr-app: samples/cbs/simple
   :board: xiao_esp32c3
   :goals: build flash
   :compact:

Sample Output
=============

The sample is built with the CBS configured with a budget of
80 milliseconds and a period of 800 milliseconds, as follows:

.. code-block:: c

  /* remember: (name, budget, period, static_priority) */
  K_CBS_DEFINE(cbs_1, K_MSEC(80), K_MSEC(800), EDF_PRIORITY);

This yields an utilization factor (bandwidth) of 80/800 = 10%
of the CPU at most, leaving the remaining 90% for whatever other
threads and CBSs the user may want to implement. The main function
simply pushes the same job three times every 2 seconds, as follows:

.. code-block:: c

  int main(void){
    for(;;){
      printf("\n");
      k_cbs_push_job(&cbs_1, job_function, job1, K_NO_WAIT);
      k_cbs_push_job(&cbs_1, job_function, job1, K_NO_WAIT);
      k_cbs_push_job(&cbs_1, job_function, job1, K_NO_WAIT);
      k_sleep(K_SECONDS(2));
    }
  }

the job itself is nothing but a function that prints some
information and busy waits for 10 milliseconds:

.. code-block:: c

  void job_function(void *arg){
    /* prints some info and increments the counter */
    printf("%s %s, %d\n\n", job->msg, CONFIG_BOARD_TARGET, job->counter);
    job->counter++;

    /* busy waits for (approximately) 10 milliseconds */
    k_busy_wait(10000);
  }

So if each job takes slightly over 10 milliseconds and the
server has an 80-millisecond budget, it is expected that all
three jobs will be served without exhausting the budget. In
other words, this would be the logging result:

.. code-block:: console

  [job]           j1 on xiao_esp32c3/esp32c3, 0

  [job]           j1 on xiao_esp32c3/esp32c3, 1

  [job]           j1 on xiao_esp32c3/esp32c3, 2

  [00:00:02.047,000] <inf> CBS: cbs_1     J_PUSH  1280000
  [00:00:02.047,000] <inf> CBS: cbs_1     B_COND  1280000   //CBS was idle. deadline = now + period
  [00:00:02.047,000] <inf> CBS: cbs_1     J_PUSH  1280000
  [00:00:02.047,000] <inf> CBS: cbs_1     J_PUSH  1280000
  [00:00:02.047,000] <inf> CBS: cbs_1     SWT_TO  1280000   //CBS enters the CPU to execute the jobs
  [00:00:02.060,000] <inf> CBS: cbs_1     J_COMP  1086468
  [00:00:02.072,000] <inf> CBS: cbs_1     J_COMP  891390
  [00:00:02.084,000] <inf> CBS: cbs_1     J_COMP  694073
  [00:00:02.084,000] <inf> CBS: cbs_1     SWT_AY  694073    //all jobs completed. CBS leaves the CPU

.. note::
  The CBS thread does an underlying conversion from timeout
  units passed on :c:macro:`K_CBS_DEFINE` (e.g. :c:macro:`K_MSEC`)
  to ensure units compatibility with :c:func:`k_thread_deadline_set`,
  which currently accepts only hardware cycles. That's why,
  in this example, K_MSEC(80) translates to 1280000 hardware
  cycles. Systems with different clock speeds will
  showcase different values.

Now what if we decrease the budget to, say, 30 milliseconds?
This would reduce the maximum CPU utilization to 30/800 = 37.5%,
but now the jobs will likely exhaust the budget and trigger a
deadline recalculation eventually. And it happens indeed:

.. code-block:: console

  [job]           j1 on xiao_esp32c3/esp32c3, 0

  [job]           j1 on xiao_esp32c3/esp32c3, 1

  [job]           j1 on xiao_esp32c3/esp32c3, 2

  [00:00:02.053,000] <inf> CBS: cbs_1     J_PUSH  480000
  [00:00:02.053,000] <inf> CBS: cbs_1     B_COND  480000  //CBS was idle. deadline = now + period
  [00:00:02.053,000] <inf> CBS: cbs_1     J_PUSH  480000
  [00:00:02.053,000] <inf> CBS: cbs_1     J_PUSH  480000
  [00:00:02.053,000] <inf> CBS: cbs_1     SWT_TO  480000
  [00:00:02.066,000] <inf> CBS: cbs_1     J_COMP  274543
  [00:00:02.078,000] <inf> CBS: cbs_1     J_COMP  81781
  [00:00:02.083,000] <inf> CBS: cbs_1     B_ROUT  480000  //CBS ran out. deadline += period
  [00:00:02.083,000] <inf> CBS: cbs_1     SWT_AY  479556  //yields for possible other threads with earlier deadline
  [00:00:02.083,000] <inf> CBS: cbs_1     SWT_TO  479556  //no other thread with earlier deadline, gets back to the CPU
  [00:00:02.090,000] <inf> CBS: cbs_1     J_COMP  370333
  [00:00:02.090,000] <inf> CBS: cbs_1     SWT_AY  370333
