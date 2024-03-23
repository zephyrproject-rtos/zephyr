.. zephyr:code-sample:: psicc2
   :name: PSiCC2 State Machine Demo
   :relevant-api: smf

   Demonstrate an event-driven hierarchical state machine.

Overview
********

This sample demonstrates the :ref:`State Machine Framework <smf>` subsystem.

Building and Running
********************

It should be possible to build and run this sample on almost any board or emulator.

Building and Running for ST Disco L475 IOT01 (B-L475E-IOT01A)
=============================================================
The sample can be built and executed for the :ref:`disco_l475_iot1_board` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/smf/psicc2
   :board: disco_l475_iot1
   :goals: build flash
   :compact:

For other boards just replace the board name.

Instructions for Use
====================
This application implements the statechart shown in Figure 2.11 of
Practical UML Statecharts in C/C++, 2nd Edition, by Miro Samek (PSiCC2). Ebook available from
https://www.state-machine.com/psicc2 This demo was chosen as it contains all possible transition
topologies up to four levels of state nesting and is used wit permission of the author.

The application logs every entry, run and exit action for each state to the console, as well as
when a state handles an event, as opposed to passing it up to the parent state.

There are two shell commands defined for controlling the operation.

* ``psicc2 event <event>`` sends the event (from A to I) to the state machine. These correspond to
  events A through I in PSiCC2 Figure 2.11
* ``psicc2 terminate`` sends the ``EVENT_TERMINATE`` event to terminate the state machine. There
  is no way to restart the state machine once terminated, and future events are ignored.

Comparison to PSiCC2 Output
===========================
Not all transitions modelled in UML may be supported by the :ref:`State Machine Framework <smf>`.
Unsupported transitions may lead to results different to the example run of the application in
PSiCC2 Section 2.3.15. The differences will not be listed here as it is hoped :ref:`SMF <smf>`
will support these transitions in the future and the list would become outdated.
