/*
 * Copyright (c) 2012, George Oikonomou - <oikonomou@users.sourceforge.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *   Trickle timer library header file.
 *
 * \author
 *   George Oikonomou - <oikonomou@users.sourceforge.net>
 */

/** \addtogroup lib
 * @{ */

/**
 * \defgroup trickle-timer Trickle timers
 *
 * This library implements timers which behave in accordance with RFC 6206
 * "The Trickle Algorithm" (http://tools.ietf.org/html/rfc6206)
 *
 * Protocols wishing to use trickle timers, may use this library instead of
 * implementing the trickle algorithm internally.
 *
 * The protocol implementation will declare one (or more) variable(s) of type
 * struct ::trickle_timer and will then populate its fields by calling
 * trickle_timer_config(). trickle_timer_set() will start the timer.
 *
 * When the timer reaches time t within the current trickle interval, the
 * library will call a protocol-provided callback, which will signal to the
 * protocol that it is time to TX (see algorithm step 4 in the RFC).
 *
 * The proto does not need to check the suppression conditions. This is done by
 * the library and if TX must be suppressed, the callback won't be called at
 * all.
 *
 * The library also provides functions to be called when the protocol hears a
 * 'consistent' or 'inconsistent' message and when an 'external event' occurs
 * (in this context, those terms have the exact same meaning as in the RFC).
 *
 * @{
 */

#ifndef TRICKLE_TIMER_H_
#define TRICKLE_TIMER_H_

#include "contiki-conf.h"
#include "sys/ctimer.h"
/*---------------------------------------------------------------------------*/
/* Trickle Timer Library Constants */
/*---------------------------------------------------------------------------*/
/**
 * \name Trickle Timer Library: Constants
 * @{
 */
/*---------------------------------------------------------------------------*/
/**
 * \brief Set as value of k to disable suppression
 */
#define TRICKLE_TIMER_INFINITE_REDUNDANCY  0x00

#define TRICKLE_TIMER_ERROR                   0
#define TRICKLE_TIMER_SUCCESS                 1

/**
 * \brief Use as the value of TRICKLE_TIMER_MAX_IMAX_WIDTH to enable the
 * generic 'Find max Imax' routine
 */
#define TRICKLE_TIMER_MAX_IMAX_GENERIC        0
/**
 * \brief Use as the value of TRICKLE_TIMER_MAX_IMAX_WIDTH to enable the 16-bit
 * 'Find max Imax' routine
 */
#define TRICKLE_TIMER_MAX_IMAX_16_BIT         1
/**
 * \brief Use as the value of TRICKLE_TIMER_MAX_IMAX_WIDTH to enable the 32-bit
 * 'Find max Imax' routine
 */
#define TRICKLE_TIMER_MAX_IMAX_32_BIT         2

/**
 * \brief Constants used as values for the \e suppress param of
 * trickle_timer_cb_t
 */
#define TRICKLE_TIMER_TX_SUPPRESS             0
#define TRICKLE_TIMER_TX_OK                   1

/**
 * \brief A trickle timer is considered 'stopped' when
 * i_cur == TRICKLE_TIMER_IS_STOPPED.
 *
 * trickle_timer_stop() must be used to correctly disable a trickle timer.
 * Do NOT set the value of i_cur to 0 directly, as this will fail to stop the
 * timer.
 */
#define TRICKLE_TIMER_IS_STOPPED              0
/** @} */
/*---------------------------------------------------------------------------*/
/**
 *  \brief Controls whether the library will try to compensate for time drifts
 *         caused by getting called later than scheduled.
 *
 * 1: Enabled (default). 0: Disabled
 *
 * To override the default, define TRICKLE_TIMER_CONF_COMPENSATE_DRIFT in
 * contiki-conf.h
 *
 * Bear in mind that this controls the behaviour of the entire library (i.e.
 * all trickle timers) and not of an individual timer
 */
#ifdef TRICKLE_TIMER_CONF_COMPENSATE_DRIFT
#define TRICKLE_TIMER_COMPENSATE_DRIFT TRICKLE_TIMER_CONF_COMPENSATE_DRIFT
#else
#define TRICKLE_TIMER_COMPENSATE_DRIFT 1
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief Turns on support for 4-byte wide, unsigned random numbers
 *
 * We need this for platforms which have a 4-byte wide clock_time_t. For those
 * platforms and if Imin << Imax is GT 0xFFFF, random_rand alone is not always
 * enough to generate a correct t in [I/2, I). Specifically, we need wide
 * randoms when I > 0x1FFFF.
 *
 * For platforms with a 2-byte wide clock_time_t, this can be defined as 0
 * to reduce code footprint and increase speed.
 */
#ifdef TRICKLE_TIMER_CONF_WIDE_RAND
#define TRICKLE_TIMER_WIDE_RAND TRICKLE_TIMER_CONF_WIDE_RAND
#else
#define TRICKLE_TIMER_WIDE_RAND 1
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief Selects a flavor for the 'Find maximum Imax' (max_imax) function.
 *
 * When configuring a new trickle timer, the library verifies that the (Imin ,
 * Imax) pair will fit in the platform's clock_time_t boundaries. If this is
 * not the case, Imax will be adjusted down. In order to achieve this, we use
 * an internal function which is a slight variant of a standard 'Count Leading
 * Zeros'.
 *
 * This functionality is disabled when ::TRICKLE_TIMER_ERROR_CHECKING is 0
 *
 * This define helps us generate, at the pre-processing stage, the desired
 * version of this function. We start with a generic version by default. The
 * platform's contiki-conf.h can change this to use the 16- or 32-bit specific
 * flavor by defining TRICKLE_TIMER_CONF_MAX_IMAX_WIDTH.
 *
 * Depending on the toolchain, the generic variant may actually result in a
 * smaller code size. Do your own experiments.
 *
 * TRICKLE_TIMER_MAX_IMAX_GENERIC (0): Generic function which will work
 * regardless whether the platform uses a 16 or 32 bit wide clock type
 *
 * TRICKLE_TIMER_MAX_IMAX_16_BIT (1): You can select this in your
 * contiki-conf.h if your platform's clock_time_t is 16 bits wide
 *
 * TRICKLE_TIMER_MAX_IMAX_32_BIT (2): You can select this in your
 * contiki-conf.h if your platform's clock_time_t is 32 bits wide
 */
#ifdef TRICKLE_TIMER_CONF_MAX_IMAX_WIDTH
#define TRICKLE_TIMER_MAX_IMAX_WIDTH TRICKLE_TIMER_CONF_MAX_IMAX_WIDTH
#else
#define TRICKLE_TIMER_MAX_IMAX_WIDTH TRICKLE_TIMER_MAX_IMAX_GENERIC
#endif

/**
 * \brief Enables/Disables error checking
 *
 * 1: Enabled (default). The library checks the validity of Imin and Imax
 * 0: Disabled. All error checking is turned off. This reduces code size.
 */
#ifdef TRICKLE_TIMER_CONF_ERROR_CHECKING
#define TRICKLE_TIMER_ERROR_CHECKING TRICKLE_TIMER_CONF_ERROR_CHECKING
#else
#define TRICKLE_TIMER_ERROR_CHECKING 1
#endif
/*---------------------------------------------------------------------------*/
/* Trickle Timer Library Macros */
/*---------------------------------------------------------------------------*/
/**
 * \name Trickle Timer Library: Macros
 * @{
 */
/**
 * \brief cross-platform method to get the maximum clock_time_t value
 */
#define TRICKLE_TIMER_CLOCK_MAX ((clock_time_t)~0)


/**
 * \brief Checks if the trickle timer's suppression feature is enabled
 *
 * \param tt A pointer to a ::trickle_timer structure
 *
 * \retval non-zero Suppression is enabled
 * \retval 0 Suppression is disabled
 */
#define TRICKLE_TIMER_SUPPRESSION_ENABLED(tt) \
  ((tt)->k != TRICKLE_TIMER_INFINITE_REDUNDANCY)

/**
 * \brief Checks if the trickle timer's suppression feature is disabled
 *
 * \param tt A pointer to a ::trickle_timer structure
 *
 * \retval non-zero Suppression is disabled
 * \retval 0 Suppression is enabled
 */
#define TRICKLE_TIMER_SUPPRESSION_DISABLED(tt) \
  ((tt)->k == TRICKLE_TIMER_INFINITE_REDUNDANCY)

/**
 * \brief Determines whether the protocol must go ahead with a transmission
 *
 * \param tt A pointer to a ::trickle_timer structure
 *
 * \retval non-zero Go ahead with TX
 * \retval 0 Suppress
 */
#define TRICKLE_TIMER_PROTO_TX_ALLOW(tt) \
  (TRICKLE_TIMER_SUPPRESSION_DISABLED(tt) || ((tt)->c < (tt)->k))

/**
 * \brief Determines whether the protocol must suppress a transmission
 *
 * \param tt A pointer to a ::trickle_timer structure
 *
 * \retval non-zero Suppress
 * \retval 0 Go ahead with TX
 */
#define TRICKLE_TIMER_PROTO_TX_SUPPRESS(tt) \
  (TRICKLE_TIMER_SUPPRESSION_ENABLED(tt) && ((tt)->c >= (tt)->k))

/**
 * \brief Returns a timer's maximum interval size (Imin << Imax) as a number of
 *        clock ticks
 *
 * \param tt A pointer to a ::trickle_timer structure
 *
 * \return Maximum trickle interval length in clock ticks
 */
#define TRICKLE_TIMER_INTERVAL_MAX(tt) ((tt)->i_max_abs)

/**
 * \brief Returns the current trickle interval's end (absolute time in ticks)
 *
 * \param tt A pointer to a ::trickle_timer structure
 *
 * \return The absolute number of clock ticks when the current trickle interval
 *         will expire
 */
#define TRICKLE_TIMER_INTERVAL_END(tt) ((tt)->i_start + (tt)->i_cur)

/**
 * \brief Checks whether an Imin value is suitable considering the various
 * restrictions imposed by our platform's clock as well as by the library itself
 *
 * \param imin An Imin value in clock ticks
 *
 * \retval 1 The Imin value is valid
 * \retval 0 The Imin value is invalid
 */
#define TRICKLE_TIMER_IMIN_IS_OK(imin) \
  ((imin > 1) && (i_min <= (TRICKLE_TIMER_CLOCK_MAX >> 1)))

/**
 * \brief Checks whether an Imin value is invalid considering the various
 * restrictions imposed by our platform's clock as well as by the library itself
 *
 * \param imin An Imin value in clock ticks
 *
 * \retval 1 The Imin value is invalid
 * \retval 0 The Imin value is valid
 */
#define TRICKLE_TIMER_IMIN_IS_BAD(imin) \
  ((imin < 2) || (i_min > (TRICKLE_TIMER_CLOCK_MAX >> 1)))

/**
 * \brief Checks whether Imin << Imax is unsuitable considering the boundaries
 *        of our platform's clock_time_t
 *
 * \param i_min Imin value
 * \param i_max Maximum number of doublings
 *
 * \retval non-zero The pair would exceed clock boundaries
 * \retval 0 The pair is suitable for the platform
 *
 * Timers scheduled far in the future can be perceived as being scheduled in
 * the past.
 * Thus, we limit Imin << Imax to be LEQ(TRICKLE_TIMER_CLOCK_MAX >> 1) + 1
 */
#define TRICKLE_TIMER_IPAIR_IS_BAD(i_min, i_max) \
  ((TRICKLE_TIMER_CLOCK_MAX >> (i_max + 1)) < i_min - 1)
/** @} */
/*---------------------------------------------------------------------------*/
/* Trickle Timer Library Data Representation */
/*---------------------------------------------------------------------------*/
/**
 * \name Trickle Timer Library: Data Representation
 * @{
 */

/**
 * \brief typedef for a callback function to be defined in the protocol's
 *        implementation.
 *
 * Those callbaks are stored as a function pointer inside a ::trickle_timer
 * structure and are called by the library at time t within the current trickle
 * interval.
 *
 * Some protocols may rely on multiple trickle timers. For this reason, this
 * function's argument will be an opaque pointer, aiming to help the protocol
 * determine which timer triggered the call.
 *
 * The \e suppress argument is used so that the library can signal the protocol
 * whether it should TX or suppress
 */
typedef void (* trickle_timer_cb_t)(void *ptr, uint8_t suppress);

/**
 * \struct trickle_timer
 *
 * A trickle timer.
 *
 * This structure is used for declaring a trickle timer. In order for the timer
 * to start running, the protocol must first populate the structure's fields
 * by calling trickle_timer_set(). Protocol implementations must NOT modify the
 * contents of this structure directly.
 *
 * Protocol developers must also be careful to specify the values of Imin and
 * Imax in such a way that the maximum interval size does not exceed the
 * boundaries of clock_time_t
 */
struct trickle_timer {
  clock_time_t i_min;     /**< Imin: Clock ticks */
  clock_time_t i_cur;     /**< I: Current interval in clock_ticks */
  clock_time_t i_start;   /**< Start of this interval (absolute clock_time) */
  clock_time_t i_max_abs; /**< Maximum interval size in clock ticks (and not in
                               number of doublings). This is a cached value of
                               Imin << Imax used internally, so that we can
                               have direct access to the maximum interval size
                               without having to calculate it all the time */
  struct ctimer ct;       /**< A \ref ctimer used internally */
  trickle_timer_cb_t cb;  /**< Protocol's own callback, invoked at time t
                               within the current interval */
  void *cb_arg;           /**< Opaque pointer to be used as the argument of the
                               protocol's callback */
  uint8_t i_max;          /**< Imax: Max number of doublings */
  uint8_t k;              /**< k: Redundancy Constant */
  uint8_t c;              /**< c: Consistency Counter */
};
/** @} */
/*---------------------------------------------------------------------------*/
/* Trickle Timer Library Functions */
/*---------------------------------------------------------------------------*/
/**
 * \name Trickle Timer Library: Functions called by protocol implementations
 * @{
 */

/**
 * \brief           Configure a trickle timer
 * \param tt        A pointer to a ::trickle_timer structure
 * \param i_min     The timer's Imin configuration parameter, in units of
 *                  clock_time_t
 * \param i_max     The timer's Imax configuration parameter (maximum number of
 *                  doublings), specified as number of doublings
 * \param k         The timer's K (redundancy constant). If the value of K
 *                  equals #TRICKLE_TIMER_INFINITE_REDUNDANCY, message
 *                  suppression will be disabled
 * \retval 0        Error (Bad argument)
 * \retval non-zero Success.
 *
 * This function is used to set the initial configuration for a trickle timer.
 * A trickle timer MUST be configured before the protocol calls
 * trickle_timer_set().
 *
 * If Imin<<Imax would exceed the platform's clock_time_t boundaries, this
 * function adjusts Imax to the maximum permitted value for the provided Imin.
 * This means that in a network with heterogenous hardware, 'we' are likely to
 * have a different Imax than 'some of them'. See RFC 6206, sec 6.3 for the
 * consequences of this situation
 */
uint8_t trickle_timer_config(struct trickle_timer *tt, clock_time_t i_min,
                             uint8_t i_max, uint8_t k);

/**
 * \brief           Start a previously configured trickle timer
 * \param tt        A pointer to a ::trickle_timer structure
 * \param proto_cb  A pointer to a callback function, which will be invoked at
 *                  at time t within the current trickle interval
 * \param ptr       An opaque pointer which will be passed as the argument to
 *                  proto_cb when the timer fires.
 * \retval 0        Error (tt was null or the timer was not configured properly)
 * \retval non-zero Success.
 *
 * This function is used to set a trickle timer. The protocol implementation
 * must use this function ONLY to initialise a new trickle timer and NOT to
 * reset it as a result of external events or inconsistencies.
 *
 * The protocol implementation must configure the trickle timer by calling
 * trickle_timer_config() before calling this function.
 */
uint8_t trickle_timer_set(struct trickle_timer *tt,
                          trickle_timer_cb_t proto_cb, void *ptr);

/**
 * \brief      Stop a running trickle timer.
 * \param tt   A pointer to a ::trickle_timer structure
 *
 * This function stops a running trickle timer that was previously started with
 * trickle_timer_start(). After this function has been called, the trickle
 * timer will no longer call the protocol's callback and its interval will not
 * double any more. In order to resume the trickle timer, the user application
 * must call trickle_timer_set().
 *
 * The protocol implementation must NOT use trickle_timer_stop(), _set() cycles
 * to reset a timer manually. Instead, in response to events or inconsistencies,
 * the corresponding functions must be used
 */
#define trickle_timer_stop(tt) do { \
  ctimer_stop(&((tt)->ct)); \
  (tt)->i_cur = TRICKLE_TIMER_IS_STOPPED; \
} while(0)

/**
 * \brief      To be called by the protocol when it hears a consistent
 *             transmission
 * \param tt   A pointer to a ::trickle_timer structure
 *
 * When the trickle-based protocol hears a consistent transmission it must call
 * this function to increment trickle's consistency counter, which is later
 * used to determine whether the protocol must suppress or go ahead with its
 * own transmissions.
 *
 * As the trickle timer library implementation may change in the future to
 * perform further tasks upon reception of a consistent transmission, the
 * protocol's implementation MUST use this call to increment the consistency
 * counter instead of directly writing to the structure's field.
 */
void trickle_timer_consistency(struct trickle_timer *tt);

/**
 * \brief      To be called by the protocol when it hears an inconsistent
 *             transmission
 * \param tt   A pointer to a ::trickle_timer structure
 *
 * When the protocol hears an inconsistent transmission, it must call this
 * function to notify the library that the timer must be reset.
 *
 * Before resetting the timer, the library will perform a set of checks.
 * Therefore, it is important that the protocol calls this function instead of
 * trying to reset the timer by itself.
 */
void trickle_timer_inconsistency(struct trickle_timer *tt);

/**
 * \brief      To be called by the protocol when an external event occurs that
 *             should trigger a timer reset
 * \param tt   A pointer to a ::trickle_timer structure
 *
 * When an external event occurs that should result in a timer reset, the
 * protocol implementation must call this function to notify the library.
 *
 * Before resetting the timer, the library will perform a set of checks.
 * Therefore, it is important that the protocol calls this function instead of
 * trying to reset the timer by itself.
 */
#define trickle_timer_reset_event(tt) trickle_timer_inconsistency(tt)

/**
 * \brief      To be called in order to determine whether a trickle timer is
 *             running
 * \param tt   A pointer to a ::trickle_timer structure
 * \retval 0   The timer is stopped
 * \retval non-zero The timer is running
 *
 */
#define trickle_timer_is_running(tt) ((tt)->i_cur != TRICKLE_TIMER_IS_STOPPED)

/** @} */

#endif /* TRICKLE_TIMER_H_ */
/** @} */
/** @} */
