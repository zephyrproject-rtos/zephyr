Work Queue (workq)
##################

The workq module provides a lightweight, single-threaded work queue for deferring tasks from
interrupt context or high-priority threads to a dedicated worker thread.



State Machines
--------------


.. mermaid::

   stateDiagram-v2
   [*] --> INITIALIZED : work_init()

   INITIALIZED --> PENDING : workq_submit()
   PENDING --> PENDING : workq_reschedule()

   PENDING --> INITIALIZED : workq_cancel()
   PENDING --> RUNNING : workq_run()

   RUNNING --> PENDING: [pending]
   RUNNING --> INITIALIZED : [!pending] release_sync()

   note right of RUNNING
     Cannot be cancelled
     The work item can be safely modified or freed.
   end note


.. mermaid::

   stateDiagram-v2
   [*] --> OPEN : workq_init()

   state OPEN {
     [*] --> OPEN_RUNNING : [!frozen]
     [*] --> OPEN_FROZEN : [frozen]
     OPEN_RUNNING --> OPEN_FROZEN : workq_freeze()
     OPEN_FROZEN --> OPEN_RUNNING : workq_thaw()

     note right of OPEN_RUNNING
         Accepting new work.
         Processing pending/delayed items.
     end note

     note right of OPEN_FROZEN
         Accepting new work.
         pending items will be processed but delayed items will not be scheduled until thawed.
     end note
   }

   state CLOSED {
     [*] --> CLOSED_RUNNING : [!frozen]
     [*] --> CLOSED_FROZEN : [frozen]
     CLOSED_RUNNING --> CLOSED_FROZEN : workq_freeze()
     CLOSED_FROZEN --> CLOSED_RUNNING : workq_thaw()

     note right of CLOSED_RUNNING
         Rejecting new work (-EAGAIN).
         Processing pending/delayed items.
     end note

     note right of CLOSED_FROZEN
         Rejecting new work (-EBUSY).
         pending items will be processed but delayed items will not be scheduled until thawed.
     end note
   }

   OPEN --> CLOSED : workq_close()
   CLOSED --> OPEN : workq_open()


.. mermaid::

   stateDiagram-v2
   [*] --> UNALLOCATED

    UNALLOCATED --> INITIALIZED : workq_thread_init()

    INITIALIZED --> RUNNING : workq_thread_start()

    RUNNING --> INITIALIZED: workq_thread_stop()


    note right of INITIALIZED
         The work queue thread is initialized but not running.
    end note
    note right of RUNNING
         The work queue thread is running and can process work items.
    end note

