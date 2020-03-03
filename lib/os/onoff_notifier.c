/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/onoff.h>
#include <stdio.h>

#define WITH_PRINTK 0

/* Identify the four external events that can trigger state changes,
 * as well as an internal state used when processing deferred actions.
 */
enum event_type {
	/* No-op event: used to process deferred changes */
	EVT_NOP,
	/* Client request for the service */
	EVT_REQUEST,
	/* Client release of the service */
	EVT_RELEASE,
	/* Client reset of the service */
	EVT_RESET,
	/* Completion of a service transition. */
	EVT_COMPLETE,
};

/* There are eight bits that indicate the state of the notifier.
 * These bits are manipulated by process_events() under lock, and
 * actions cued by bit values are executed outside of lock within
 * process_events().
 *
 * * ON specifies the target state for the client: clear for OFF, set
 *   for ON.
 * * RESET specifies that the notifier is being reset.
 * * CHANGING specifies that the device is in transition to the target
 *   state.  This bit is set when a request or release event occurs
 *   while the notifier is in a stable state, and is cleared when the
 *   required onoff transition has been initiated.
 * * CANCEL specifies that the target state changed while a transition
 *   had not been completed.  Set or cleared based on request and
 *   release events that occur while CHANGING is set, cleared when the
 *   completion event is received.
 * * UNSUBMITTED indicates that a required onoff transition has not
 *   yet been initiated.  This is set when CHANGING is set, and is
 *   cleared when mutex is restored after initiating the onoff
 *   transition.
 * * UNNOTIFIED indicates that an error or transition completion has
 *   been recorded but the notifier callback has not yet been invoked.
 *   Set when an error is detected or on completion of an uncancelled
 *   transition, cleared when the notification is about to be invoked.
 * * PROCESSING is used to mark that there is an active invocation of
 *   the state machine.  It defers actions related to state changes
 *   that occur during the unlocked periods where onoff transition or
 *   notification occurs, which simplifies the state machine.  The
 *   actions will be invoked when the top-level process_events()
 *   invocation regains control.
 * * ERROR indicates receipt of an error from the underlying onoff
 *   service, either a rejected transition or a failed transition.
 */
#define ST_BIT_ON BIT(0)
#define ST_BIT_RESET BIT(1)
#define ST_BIT_CHANGING BIT(2)
#define ST_BIT_CANCEL BIT(3)
#define ST_BIT_UNSUBMITTED BIT(4)
#define ST_BIT_UNNOTIFIED BIT(5)
#define ST_BIT_PROCESSING BIT(6)
#define ST_BIT_ERROR BIT(7)

#define ST_OFF 0
#define ST_ON ST_BIT_ON
#define ST_CHANGING_FROM_OFF (ST_OFF | ST_BIT_CHANGING)
#define ST_CHANGING_FROM_ON (ST_ON | ST_BIT_CHANGING)
#define ST_RESETTING (ST_BIT_RESET | ST_BIT_CHANGING)

/* Mask used to isolate the bits required to the core state: stable ON
 * and OFF, and unstable changing from ON or OFF.
 */
#define ST_CHANGING_MASK (ST_BIT_ON | ST_BIT_RESET | ST_BIT_CHANGING)

static inline bool state_has_error(u32_t state)
{
	return state & ST_BIT_ERROR;
}

static inline u32_t state_set_error(struct onoff_notifier *np,
				    int res)
{
	u32_t state = np->state;

	np->onoff_result = res;
	state |= ST_BIT_ERROR | ST_BIT_UNNOTIFIED;
	np->state = state;

	return state;
}

#if (WITH_PRINTK - 0)
#define PRINTK(...) printk(__VA_ARGS__)

static const char *const evt_s[] = {
	[EVT_NOP] = "nop",
	[EVT_REQUEST] = "request",
	[EVT_RELEASE] = "release",
	[EVT_RESET] = "reset",
	[EVT_COMPLETE] = "complete",
};

static const char *state_s(u32_t state)
{
	static char buf[128];

	snprintf(buf, sizeof(buf), "%x: %s%s%s%s%s%s%s",
		 state,
		 (state & ST_BIT_ON) ? "ON" : "off",
		 (state & ST_BIT_CHANGING) ? " CHANGING" : "",
		 (state & ST_BIT_CANCEL) ? " CANCEL" : "",
		 (state & ST_BIT_UNSUBMITTED) ? " UNSUBMITTED" : "",
		 (state & ST_BIT_UNNOTIFIED) ? " UNNOTIFIED" : "",
		 (state & ST_BIT_PROCESSING) ? " PROCESSING" : "",
		 (state & ST_BIT_ERROR) ? " ERROR" : "");
	return buf;
}

#else /* WITH_PRINTK */
#define PRINTK(...) (void)0
#endif /* WITH_PRINTK */

static int process_event(struct onoff_notifier *np,
			 int evt,
			 int res);

static void onoff_callback(struct onoff_manager *mp,
			   struct onoff_client *cli,
			   u32_t state,
			   int res,
			   void *user_data)
{
	struct onoff_notifier *np = user_data;

	process_event(np, EVT_COMPLETE, res);
}

int onoff_notifier_request(struct onoff_notifier *np)
{
	return process_event(np, EVT_REQUEST, 0);
}

int onoff_notifier_release(struct onoff_notifier *np)
{
	return process_event(np, EVT_RELEASE, 0);
}

int onoff_notifier_reset(struct onoff_notifier *np)
{
	return process_event(np, EVT_RESET, 0);
}

static void issue_change(struct onoff_notifier *np,
			 k_spinlock_key_t *key)
{
	int rc;
	u32_t mode = np->state & ST_CHANGING_MASK;

	PRINTK("submit %s\n", state_s(mode));
	k_spin_unlock(&np->lock, *key);

	onoff_client_init_callback(&np->cli, onoff_callback, np);
	if (mode == ST_CHANGING_FROM_OFF) {
		rc = onoff_request(np->onoff, &np->cli);
	} else if (mode == ST_CHANGING_FROM_ON) {
		rc = onoff_release(np->onoff, &np->cli);
	} else {
		__ASSERT_NO_MSG(mode == ST_RESETTING);
		rc = onoff_reset(np->onoff, &np->cli);
		if (rc == -EALREADY) {
			/* Somebody already cleared the onoff service
			 * error; synthesize a successful completion
			 * so the notifier state gets reset.
			 */
			rc = 0;
			process_event(np, EVT_COMPLETE, rc);
		}
	}

	*key = k_spin_lock(&np->lock);
	PRINTK("submitted %s %d\n", state_s(np->state), rc);
	if (rc < 0) {
		state_set_error(np, rc);
	}
}

static void issue_notification(struct onoff_notifier *np,
			       k_spinlock_key_t *key)
{
	u32_t state = np->state & ST_CHANGING_MASK;
	int res = np->onoff_result;

	PRINTK("notify %x\n", np->state);
	k_spin_unlock(&np->lock, *key);

	if (res >= 0) {
		res = (state == ST_BIT_ON);
	}
	np->callback(np, res);

	*key = k_spin_lock(&np->lock);
	PRINTK("notified %s\n", state_s(np->state));
}

/* There are two points in the state machine where the machine is
 * unlocked to perform some external action:
 * * Initiation of an onoff transition due to some event;
 * * Invocation of the user-specified callback when a stable state is
 *   reached or an error detected.
 *
 * Events received during these unlocked periods are recorded in the
 * state, but processing is deferred to the top-level invocation which
 * will loop to handle any events that occurred during the unlocked
 * regions.
 */
static int process_event(struct onoff_notifier *np,
			 int evt,
			 int res)
{
	static unsigned int depth;
	int rc = 0;
	bool loop = false;
	k_spinlock_key_t key = k_spin_lock(&np->lock);
	u32_t state = np->state;

	++depth;
	PRINTK("pev[%d] entry %s %s\n", depth, state_s(state), evt_s[evt]);
	__ASSERT_NO_MSG(evt != EVT_NOP);
	do {
		rc = 0;
		loop = false;

		if (state_has_error(state)
		    && (evt != EVT_RESET)) {
			rc = -EIO;
		} else if (((state & ST_CHANGING_MASK) == ST_RESETTING)
			   && (evt != EVT_COMPLETE)) {
			rc = -EWOULDBLOCK;
		} else if (evt == EVT_REQUEST) {
			if ((state & ST_CHANGING_MASK) == ST_OFF) {
				state = ST_CHANGING_FROM_OFF
					| ST_BIT_UNSUBMITTED;
			} else if ((state & ST_CHANGING_MASK)
				   == ST_CHANGING_FROM_OFF) {
				state &= ~ST_BIT_CANCEL;
			} else if ((state & ST_CHANGING_MASK)
				   == ST_CHANGING_FROM_ON) {
				state |= ST_BIT_CANCEL;
			} else {
				rc = -EALREADY;
			}
		} else if (evt == EVT_RELEASE) {
			if ((state & ST_CHANGING_MASK) == ST_ON) {
				state = ST_CHANGING_FROM_ON
					| ST_BIT_UNSUBMITTED;
			} else if ((state & ST_CHANGING_MASK)
				   == ST_CHANGING_FROM_ON) {
				state &= ~ST_BIT_CANCEL;
			} else if ((state & ST_CHANGING_MASK)
				   == ST_CHANGING_FROM_OFF) {
				state |= ST_BIT_CANCEL;
			} else {
				rc = -EALREADY;
			}
		} else if (evt == EVT_RESET) {
			if (state_has_error(state)) {
				state &= ~(ST_BIT_ON | ST_BIT_ERROR);
				state |= ST_BIT_RESET | ST_BIT_CHANGING
					 | ST_BIT_UNSUBMITTED;
			} else {
				rc = -EALREADY;
			}
		} else if (evt == EVT_COMPLETE) {
			u32_t mode = state & ST_CHANGING_MASK;

			__ASSERT_NO_MSG((state & ST_BIT_CHANGING) != 0U);
			state &= ~ST_BIT_CHANGING;
			PRINTK("pev[%d] complete %s %d\n", depth,
			       state_s(state), res);
			np->onoff_result = res;
			if (res < 0) {
				np->state = state;
				state = state_set_error(np, res);
			} else if (mode == ST_RESETTING) {
				/* Reset completed: return to OFF
				 * state and notify.
				 */
				state = ST_OFF | ST_BIT_UNNOTIFIED;
			} else {
				bool cancel = ((state & ST_BIT_CANCEL) != 0U);

				/* The completed operation inverts the
				 * on bit and clears any pending
				 * cancel.  Loop back to issue the
				 * reverse transition if the operation
				 * is to be cancelled, otherwise
				 * notify the client.
				 */
				state ^= ST_BIT_ON;
				state &= ~ST_BIT_CANCEL;

				if (cancel) {
					loop = true;
					evt = (state == ST_ON)
					      ? EVT_RELEASE
					      : EVT_REQUEST;
				} else {
					state |= ST_BIT_UNNOTIFIED;
				}
				PRINTK("pev[%d] settle %s : %s\n", depth,
				       state_s(state),
				       cancel ? " CANCEL" : "settle");

			}
		} else {
			__ASSERT_NO_MSG(evt == EVT_NOP);
		}

		if (!loop) {
			evt = EVT_NOP;
		}

		/* If we're in a nested call defer any additional processing. */
		if ((state & ST_BIT_PROCESSING) != 0U) {
			break;
		}

		state |= ST_BIT_PROCESSING;

		/* Initiate any unsubmitted onoff transition. */
		if ((state & ST_BIT_UNSUBMITTED) != 0U) {
			state &= ~ST_BIT_UNSUBMITTED;
			np->state = state;

			issue_change(np, &key);

			loop |= (np->state != state);
			state = np->state;
		}

		/* Initiate any unnotified notifications. */
		if ((state & ST_BIT_UNNOTIFIED) != 0U) {
			state &= ~ST_BIT_UNNOTIFIED;
			np->state = state;

			issue_notification(np, &key);

			loop |= (np->state != state);
			state = np->state;
		}

		state &= ~ST_BIT_PROCESSING;
	} while (loop);

	np->state = state;
	if (rc >= 0) {
		rc = ((state & ST_CHANGING_MASK) == ST_BIT_ON) != 0U;
	}


	PRINTK("pev[%d] exit %s %d\n", depth, state_s(state), rc);
	--depth;
	k_spin_unlock(&np->lock, key);

	return rc;
}
