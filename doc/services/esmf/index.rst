.. _esmf:

Event-driven State Machine Framework (ESMF)
###########################################

.. highlight:: c

Overview
========

The Event-driven State Machine Framework (ESMF) is a thin, table-driven event
dispatcher built on top of the :ref:`smf`. It is intended for state machines
where transitions are driven exclusively by discrete events rather than by a
periodic run action.

ESMF can be added to any project by enabling the
:kconfig:option:`CONFIG_SMF_ESMF` option, which implies :kconfig:option:`CONFIG_SMF`.

Transition Table
================

The core concept in ESMF is a static array of :c:struct:`esmf_transition`
entries. Each entry describes:

* **from** — the source state, or ``NULL`` to match any current state.
* **trigger** — the event identifier to match, or :c:macro:`ESMF_TRIGGER_ANY`
  to match any event.
* **guard** — an optional :c:type:`esmf_guard_t` callback. If provided, the
  transition is only taken when the guard returns ``true``.
* **action** — an optional :c:type:`esmf_action_t` callback executed before
  the state change.
* **to** — the destination state, or ``NULL`` for an internal transition that
  does not change state.

Use the :c:macro:`ESMF_CREATE_TRANSITION` helper macro to populate the table::

   static const struct esmf_transition my_transitions[] = {
      ESMF_CREATE_TRANSITION(&states[S0], EV_GO, NULL,     NULL,   &states[S1]),
      ESMF_CREATE_TRANSITION(&states[S1], EV_OK, my_guard, my_act, &states[S2]),
      ESMF_CREATE_TRANSITION(NULL,        EV_RST, NULL,    NULL,   &states[S0]),
   };

If explicit priorities are needed, use :c:macro:`ESMF_CREATE_TRANSITION_PRIO`.
Lower numeric values have higher priority::

   static const struct esmf_transition my_prio_transitions[] = {
      ESMF_CREATE_TRANSITION_PRIO(&states[S0], EV_WARN, NULL, NULL, &states[S1], 10),
      ESMF_CREATE_TRANSITION_PRIO(&states[S0], EV_WARN, NULL, NULL, &states[S2],  1),
   };

Self-transitions
----------------

When the destination state equals the current state, ESMF calls
:c:func:`smf_set_state` as normal, so the state's **exit and entry actions are
re-executed**, matching SMF self-transition semantics. This differs from an
internal transition (``to = NULL``), where no state change at all is
performed.

Event Matching and Priority
===========================

:c:func:`esmf_handle_event` iterates the transition table in order and evaluates
all matching transitions whose guards (if any) return ``true``. Among those,
the transition with the lowest priority value is selected. If multiple
transitions have the same priority, the first one found in table order wins.

If one or more entries matched the current state and trigger but all guards
rejected the transition, the function returns ``-EACCES``.  If no entry matched
at all, it returns ``-ENOENT``.

For external transitions (``to != NULL``), SMF entry and exit actions are
executed according to :c:func:`smf_set_state` semantics.

:c:func:`esmf_handle_event` does not execute SMF run actions. SMF run actions
should be ``NULL`` for pure event-driven ESMF state machines, or handled
separately outside ESMF event dispatch.

Initialization and Execution
=============================

Initialize the ESMF context with :c:func:`esmf_init`, then dispatch events
using :c:func:`esmf_handle_event`::

   struct my_obj {
      struct esmf_ctx esmf;
      bool ready;
   };

   static struct my_obj obj;

   static bool my_guard(struct esmf_ctx *ctx, uint32_t trigger)
   {
      struct my_obj *obj = (struct my_obj *)ctx;

      ARG_UNUSED(trigger);
      return obj->ready;
   }

   void app_init(void)
   {
      esmf_init(ESMF_CTX(&obj), my_transitions, ARRAY_SIZE(my_transitions));
      smf_set_initial(SMF_CTX(&obj), &states[S0]);
   }

   void app_on_event(uint32_t ev)
   {
      int rc = esmf_handle_event(ESMF_CTX(&obj), ev);
      if (rc == -ENOENT) {
         /* unhandled event */
      }
   }

API Reference
=============

.. doxygengroup:: esmf
