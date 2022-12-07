.. _smf:

State Machine Framework
#######################

Overview
========

The State Machine Framework (SMF) is an application agnostic framework that
provides an easy way for developers to integrate state machines into their
application. The framework can be added to any project by enabling the
:kconfig:option:`CONFIG_SMF` option.

State Creation
==============

A state is represented by three functions, where one function implements the
Entry actions, another function implements the Run actions, and the last
function implements the Exit actions. The prototype for these functions is as
follows: ``void funct(void *obj)``, where the ``obj`` parameter is a user
defined structure that has the state machine context, ``struct smf_ctx``, as
its first member. For example::

   struct user_object {
      struct smf_ctx ctx;
      /* All User Defined Data Follows */
   };

The ``struct smf_ctx`` member must be first because the state machine
framework's functions casts the user defined object to the ``struct smf_ctx``
type with the following macro: ``SMF_CTX(o)``

For example instead of doing this ``(struct smf_ctx *)&user_obj``, you could
use ``SMF_CTX(&user_obj)``.

By default, a state can have no ancestor states, resulting in a flat state
machine. But to enable the creation of a hierarchical state machine, the
:kconfig:option:`CONFIG_SMF_ANCESTOR_SUPPORT` option must be enabled.

The following macro can be used for easy state creation:

* :c:macro:`SMF_CREATE_STATE` Create a state

**NOTE:** The :c:macro:`SMF_CREATE_STATE` macro takes an additional parameter
when :kconfig:option:`CONFIG_SMF_ANCESTOR_SUPPORT` is enabled.

State Machine Creation
======================

A state machine is created by defining a table of states that's indexed by an
enum. For example, the following creates three flat states::

   enum demo_state { S0, S1, S2 };

   const struct smf_state demo_states[] = {
      [S0] = SMF_CREATE_STATE(s0_entry, s0_run, s0_exit),
      [S1] = SMF_CREATE_STATE(s1_entry, s1_run, s1_exit),
      [S2] = SMF_CREATE_STATE(s2_entry, s2_run, s2_exit)
   };

And this example creates three hierarchical states::

   enum demo_state { S0, S1, S2 };

   const struct smf_state demo_states[] = {
      [S0] = SMF_CREATE_STATE(s0_entry, s0_run, s0_exit, parent_s0),
      [S1] = SMF_CREATE_STATE(s1_entry, s1_run, s1_exit, parent_s12),
      [S2] = SMF_CREATE_STATE(s2_entry, s2_run, s2_exit, parent_s12)
   };


To set the initial state, the ``smf_set_initial`` function should be
called. It has the following prototype:
``void smf_set_initial(smf_ctx *ctx, smf_state *state)``

To transition from one state to another, the ``smf_set_state`` function is
used and it has the following prototype:
``void smf_set_state(smf_ctx *ctx, smf_state *state)``

**NOTE:** While the state machine is running, smf_set_state should only be
called from the Entry and Run functions. Calling smf_set_state from the Exit
functions doesn't make sense and will generate a warning.

State Machine Execution
=======================

To run the state machine, the ``smf_run_state`` function should be called in
some application dependent way. An application should cease calling
smf_run_state if it returns a non-zero value. The function has the following
prototype: ``int32_t smf_run_state(smf_ctx *ctx)``

State Machine Termination
=========================

To terminate the state machine, the ``smf_terminate`` function should be
called. It can be called from the entry, run, or exit action. The function
takes a non-zero user defined value that's returned by the ``smf_run_state``
function. The function has the following prototype:
``void smf_terminate(smf_ctx *ctx, int32_t val)``

Flat State Machine Example
==========================

This example turns the following state diagram into code using the SMF, where
the initial state is S0.

.. graphviz::
   :caption: Flat state machine diagram

   digraph smf_flat {
      node [style=rounded];
      init [shape = point];
      STATE_S0 [shape = box];
      STATE_S1 [shape = box];
      STATE_S2 [shape = box];

      init -> STATE_S0;
      STATE_S0 -> STATE_S1;
      STATE_S1 -> STATE_S2;
      STATE_S2 -> STATE_S0;
   }

Code::

	#include <zephyr/smf.h>

	/* Forward declaration of state table */
	static const struct smf_state demo_states[];

	/* List of demo states */
	enum demo_state { S0, S1, S2 };

	/* User defined object */
	struct s_object {
		/* This must be first */
		struct smf_ctx ctx;

		/* Other state specific data add here */
	} s_obj;

	/* State S0 */
	static void s0_entry(void *o)
	{
		/* Do something */
	}
	static void s0_run(void *o)
	{
		smf_set_state(SMF_CTX(&s_obj), &demo_states[S1]);
	}
	static void s0_exit(void *o)
	{
		/* Do something */
	}

	/* State S1 */
	static void s1_run(void *o)
	{
		smf_set_state(SMF_CTX(&s_obj), &demo_states[S2]);
	}
	static void s1_exit(void *o)
	{
		/* Do something */
	}

	/* State S2 */
	static void s2_entry(void *o)
	{
		/* Do something */
	}
	static void s2_run(void *o)
	{
		smf_set_state(SMF_CTX(&s_obj), &demo_states[S0]);
	}

	/* Populate state table */
	static const struct smf_state demo_states[] = {
		[S0] = SMF_CREATE_STATE(s0_entry, s0_run, s0_exit),
		/* State S1 does not have an entry action */
		[S1] = SMF_CREATE_STATE(NULL, s1_run, s1_exit),
		/* State S2 does not have an exit action */
		[S2] = SMF_CREATE_STATE(s2_entry, s2_run, NULL),
	};

	void main(void)
	{
		int32_t ret;

		/* Set initial state */
		smf_set_initial(SMF_CTX(&s_obj), &demo_states[S0]);

		/* Run the state machine */
		while(1) {
			/* State machine terminates if a non-zero value is returned */
			ret = smf_run_state(SMF_CTX(&s_obj));
			if (ret) {
				/* handle return code and terminate state machine */
				break;
			}
			k_msleep(1000);
		}
	}

Hierarchical State Machine Example
==================================

This example turns the following state diagram into code using the SMF, where
S0 and S1 share a parent state and S0 is the initial state.


.. graphviz::
   :caption: Hierarchical state machine diagram

   digraph smf_hierarchical {
      node [style = rounded];
      init [shape = point];
      STATE_S0 [shape = box];
      STATE_S1 [shape = box];
      STATE_S2 [shape = box];

      subgraph cluster_0 {
         label = "PARENT";
         style = rounded;
         STATE_S0 -> STATE_S1;
      }

      init -> STATE_S0;
      STATE_S1 -> STATE_S2;
      STATE_S2 -> STATE_S0;
   }

Code::

	#include <zephyr/smf.h>

	/* Forward declaration of state table */
	static const struct smf_state demo_states[];

	/* List of demo states */
	enum demo_state { PARENT, S0, S1, S2 };

	/* User defined object */
	struct s_object {
		/* This must be first */
		struct smf_ctx ctx;

		/* Other state specific data add here */
	} s_obj;

	/* Parent State */
	static void parent_entry(void *o)
	{
		/* Do something */
	}
	static void parent_exit(void *o)
	{
		/* Do something */
	}

	/* State S0 */
	static void s0_run(void *o)
	{
		smf_set_state(SMF_CTX(&s_obj), &demo_states[S1]);
	}

	/* State S1 */
	static void s1_run(void *o)
	{
		smf_set_state(SMF_CTX(&s_obj), &demo_states[S2]);
	}

	/* State S2 */
	static void s2_run(void *o)
	{
		smf_set_state(SMF_CTX(&s_obj), &demo_states[S0]);
	}

	/* Populate state table */
	static const struct smf_state demo_states[] = {
		/* Parent state does not have a run action */
		[PARENT] = SMF_CREATE_STATE(parent_entry, NULL, parent_exit, NULL),
		/* Child states do not have entry or exit actions */
		[S0] = SMF_CREATE_STATE(NULL, s0_run, NULL, &demo_states[PARENT]),
		[S1] = SMF_CREATE_STATE(NULL, s1_run, NULL, &demo_states[PARENT]),
		/* State S2 do ot have entry or exit actions and no parent */
		[S2] = SMF_CREATE_STATE(NULL, s2_run, NULL, NULL),
	};

	void main(void)
	{
		int32_t ret;

		/* Set initial state */
		smf_set_initial(SMF_CTX(&s_obj), &demo_states[S0]);

		/* Run the state machine */
		while(1) {
			/* State machine terminates if a non-zero value is returned */
			ret = smf_run_state(SMF_CTX(&s_obj));
			if (ret) {
				/* handle return code and terminate state machine */
				break;
			}
			k_msleep(1000);
		}
	}

When designing hierarchical state machines, the following should be considered:
 - Ancestor entry actions are executed before the sibling entry actions. For
   example, the parent_entry function is called before the s0_entry function.
 - Transitioning from one sibling to another with a shared ancestry does not
   re-execute the ancestor\'s entry action or execute the exit action.
   For example, the parent_entry function is not called when transitioning
   from S0 to S1, nor is the parent_exit function called.
 - Ancestor exit actions are executed after the sibling exit actions. For
   example, the s1_exit function is called before the parent_exit function
   is called.
 - The parent_run function only executes if the child_run function returns
   without transitioning to another state, ie. calling smf_set_state.

Event Driven State Machine Example
==================================

Events are not explicitly part of the State Machine Framework but an event driven
state machine can be implemented using Zephyr :ref:`events`.

.. graphviz::
   :caption: Event driven state machine diagram

   digraph smf_flat {
      node [style=rounded];
      init [shape = point];
      STATE_S0 [shape = box];
      STATE_S1 [shape = box];

      init -> STATE_S0;
      STATE_S0 -> STATE_S1 [label = "BTN EVENT"];
      STATE_S1 -> STATE_S0 [label = "BTN EVENT"];
   }

Code::

	#include <zephyr/kernel.h>
	#include <zephyr/drivers/gpio.h>
	#include <zephyr/smf.h>

	#define SW0_NODE        DT_ALIAS(sw0)

	/* List of events */
	#define EVENT_BTN_PRESS BIT(0)

	static const struct gpio_dt_spec button =
		GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});

	static struct gpio_callback button_cb_data;

	/* Forward declaration of state table */
	static const struct smf_state demo_states[];

	/* List of demo states */
	enum demo_state { S0, S1 };

	/* User defined object */
	struct s_object {
		/* This must be first */
		struct smf_ctx ctx;

		/* Events */
		struct k_event smf_event;
		int32_t events;

		/* Other state specific data add here */
	} s_obj;

	/* State S0 */
	static void s0_entry(void *o)
	{
		printk("STATE0\n");
	}

	static void s0_run(void *o)
	{
		struct s_object *s = (struct s_object *)o;

		/* Change states on Button Press Event */
		if (s->events & EVENT_BTN_PRESS) {
			smf_set_state(SMF_CTX(&s_obj), &demo_states[S1]);
		}
	}

	/* State S1 */
	static void s1_entry(void *o)
	{
		printk("STATE1\n");
	}

	static void s1_run(void *o)
	{
		struct s_object *s = (struct s_object *)o;

		/* Change states on Button Press Event */
		if (s->events & EVENT_BTN_PRESS) {
			smf_set_state(SMF_CTX(&s_obj), &demo_states[S0]);
		}
	}

	/* Populate state table */
	static const struct smf_state demo_states[] = {
		[S0] = SMF_CREATE_STATE(s0_entry, s0_run, NULL),
		[S1] = SMF_CREATE_STATE(s1_entry, s1_run, NULL),
	};

	void button_pressed(const struct device *dev,
			struct gpio_callback *cb, uint32_t pins)
	{
		/* Generate Button Press Event */
		k_event_post(&s_obj.smf_event, EVENT_BTN_PRESS);
	}

	void main(void)
	{
		int ret;

		if (!device_is_ready(button.port)) {
			printk("Error: button device %s is not ready\n",
				button.port->name);
			return;
		}

		ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure %s pin %d\n",
				ret, button.port->name, button.pin);
			return;
		}

		ret = gpio_pin_interrupt_configure_dt(&button,
			GPIO_INT_EDGE_TO_ACTIVE);
		if (ret != 0) {
			printk("Error %d: failed to configure interrupt on %s pin %d\n",
				ret, button.port->name, button.pin);
			return;
		}

		gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
		gpio_add_callback(button.port, &button_cb_data);

		/* Initialize the event */
		k_event_init(&s_obj.smf_event);

		/* Set initial state */
		smf_set_initial(SMF_CTX(&s_obj), &demo_states[S0]);

		/* Run the state machine */
		while(1) {
			/* Block until an event is detected */
			s_obj.events = k_event_wait(&s_obj.smf_event,
					EVENT_BTN_PRESS, true, K_FOREVER);

			/* State machine terminates if a non-zero value is returned */
			ret = smf_run_state(SMF_CTX(&s_obj));
			if (ret) {
				/* handle return code and terminate state machine */
				break;
			}
		}
	}
