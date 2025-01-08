.. zephyr:code-sample-category:: cbs_samples
   :name: Constant Bandwidth Server (CBS)
   :show-listing:

   Samples that demonstrate the usage of the Constant Bandwidth Server (CBS).
   The CBS is an extension of the Earliest Deadline First (EDF) scheduler
   that tasks that automatically recalculates their deadlines when they
   exceed their configured timing constraints. If a task takes longer than
   expected, it can be preempted by awaiting tasks instead of jeopardizing
   their deadlines.
