.. zephyr:code-sample:: esmf_traffic_light
   :name: ESMF Traffic Light
   :relevant-api: smf

   Demonstrate event-driven state transitions using ESMF priorities and guards.

Overview
********

This sample demonstrates the Event-driven State Machine Framework (ESMF),
which is built on top of :ref:`State Machine Framework <smf>`.

The demo models a hierarchical traffic light with these states:

- ``OPERATIONAL`` (parent state)
- ``RED``
- ``GREEN``
- ``YELLOW``
- ``FLASHING_RED``

``RED``, ``GREEN``, and ``YELLOW`` are children of ``OPERATIONAL``.

The application handles events using :c:func:`esmf_handle_event` and shows:

- Priority selection: lower numeric priority wins.
- Tie handling: equal priority keeps first transition found in the table.
- Guards: pedestrian requests are only accepted once minimum green time elapsed.
- Internal transitions: housekeeping action without changing state.

Queue-based multi-threading
===========================

This sample uses a message queue to keep ESMF state handling single-threaded
while accepting events from multiple threads.

- Producers:
   - Shell command handler posts user events.
   - Traffic controller thread posts periodic automatic events.
- Consumer:
   - Main thread blocks on the queue and is the only thread that calls
      :c:func:`esmf_handle_event`.

This avoids concurrent access to the SMF/ESMF context and keeps state
transitions deterministic.

A shell command is provided to inject events manually.

Statechart (PlantUML)
=====================

.. code-block:: text

   @startuml
   hide empty description

    [*] --> RED

    state OPERATIONAL {
       RED --> GREEN : TIMER
       GREEN --> YELLOW : TIMER
       YELLOW --> RED : TIMER

       GREEN --> YELLOW : PED_BUTTON\n[green_ticks >= 2]
    }

    OPERATIONAL --> FLASHING_RED : EMERGENCY\nprio = -10
    FLASHING_RED --> FLASHING_RED : EMERGENCY\nprio = -10

   FLASHING_RED --> RED : RESET

   RED --> RED : RESET\nprio = 20 (first found wins)
   RED --> GREEN : RESET\nprio = 20

   OPERATIONAL --> OPERATIONAL : DIAG_TICK / diag_action
   FLASHING_RED --> FLASHING_RED : DIAG_TICK / diag_action
   @enduml

Building and Running
********************

This sample is intended to run on almost any board and on native simulator.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/smf/esmf_traffic_light
   :board: native_sim/native/64
   :goals: build run
   :compact:

Shell usage
***********

Use the shell command:

- ``traffic event <name>``

Supported event names:

- ``timer``
- ``ped``
- ``emergency``
- ``diag``
- ``reset``

``tick`` is intentionally not exposed through shell commands; it is generated
only by the traffic controller thread.

Example:

- ``traffic event emergency``
- ``traffic event reset``
