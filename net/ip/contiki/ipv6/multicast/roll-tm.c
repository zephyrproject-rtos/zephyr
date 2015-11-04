/*
 * Copyright (c) 2010, Loughborough University - Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \addtogroup roll-tm
 * @{
 */
/**
 * \file
 *    Implementation of the ROLL TM multicast engine
 * \author
 *    George Oikonomou - <oikonomou@users.sourceforge.net>
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "contiki/ipv6/uip-icmp6.h"
#include "contiki/ipv6/multicast/uip-mcast6.h"
#include "contiki/ipv6/multicast/roll-tm.h"
#include "dev/watchdog.h"
#include <string.h>

#define DEBUG DEBUG_NONE
#include "contiki/ip/uip-debug.h"

#define TRICKLE_VERBOSE 0

#if DEBUG && TRICKLE_VERBOSE
#define VERBOSE_PRINTF(...) PRINTF(__VA_ARGS__)
#define VERBOSE_PRINT_SEED(s) PRINT_SEED(s)
#else
#define VERBOSE_PRINTF(...)
#define VERBOSE_PRINT_SEED(...)
#endif

/*---------------------------------------------------------------------------*/
/* Data Representation */
/*---------------------------------------------------------------------------*/
#if ROLL_TM_SHORT_SEEDS
typedef union seed_id_u {
  uint8_t u8[2];
  uint16_t id;                  /* Big Endian */
} seed_id_t;

#define seed_is_null(s) ((s)->id == 0)
#define PRINT_SEED(s) PRINTF("0x%02x%02x", (s)->u8[0], (s)->u8[1])
#else /* ROLL_TM_SHORT_SEEDS */
typedef uip_ip6addr_t seed_id_t;

#define seed_is_null(s) uip_is_addr_unspecified(s)
#define PRINT_SEED(s) PRINT6ADDR(s)
#endif /* ROLL_TM_SHORT_SEEDS */
#define seed_id_cmp(a, b) (memcmp((a), (b), sizeof(seed_id_t)) == 0)
#define seed_id_cpy(a, b) (memcpy((a), (b), sizeof(seed_id_t)))

/* Trickle Timers */
struct trickle_param {
  clock_time_t i_min;           /* Clock ticks */
  clock_time_t t_start;         /* Start of the interval (absolute clock_time) */
  clock_time_t t_end;           /* End of the interval (absolute clock_time) */
  clock_time_t t_next;          /* Clock ticks, randomised in [I/2, I) */
  clock_time_t t_last_trigger;
  struct ctimer ct;
  uint8_t i_current;            /* Current doublings from i_min */
  uint8_t i_max;                /* Max number of doublings */
  uint8_t k;                    /* Redundancy Constant */
  uint8_t t_active;             /* Units of Imax */
  uint8_t t_dwell;              /* Units of Imax */
  uint8_t c;                    /* Consistency Counter */
  uint8_t inconsistency;
};

/**
 * \brief Convert a timer to a sane clock_time_t value after d doublings
 * m is a value of Imin, d is a number of doublings
 * Careful of overflows
 */
#define TRICKLE_TIME(m, d) ((clock_time_t)((m) << (d)))

/**
 * \brief Convert Imax from number of doublings to clock_time_t units for
 * trickle_param t. Again, watch out for overflows */
#define TRICKLE_IMAX(t) ((uint32_t)((t)->i_min << (t)->i_max))

/**
 * \brief Convert Tactive for a trickle timer to a sane clock_time_t value
 * t is a pointer to the timer
 * Careful of overflows
 */
#define TRICKLE_ACTIVE(t) ((uint32_t)(TRICKLE_IMAX(t) * t->t_active))

/**
 * \brief Convert Tdwell for a trickle timer to a sane clock_time_t value
 * t is a pointer to the timer
 * Careful of overflows
 */
#define TRICKLE_DWELL(t) ((uint32_t)(TRICKLE_IMAX(t) * t->t_dwell))

/**
 * \brief Check if suppression is enabled for trickle_param t
 * t is a pointer to the timer
 */
#define SUPPRESSION_ENABLED(t) ((t)->k != ROLL_TM_INFINITE_REDUNDANCY)

/**
 * \brief Check if suppression is disabled for trickle_param t
 * t is a pointer to the timer
 */
#define SUPPRESSION_DISABLED(t) ((t)->k == ROLL_TM_INFINITE_REDUNDANCY)

/**
 * \brief Init trickle_timer[m]
 */
#define TIMER_CONFIGURE(m) do { \
  t[m].i_min = ROLL_TM_IMIN_##m; \
  t[m].i_max = ROLL_TM_IMAX_##m; \
  t[m].k = ROLL_TM_K_##m; \
  t[m].t_active = ROLL_TM_T_ACTIVE_##m; \
  t[m].t_dwell = ROLL_TM_T_DWELL_##m; \
  t[m].t_last_trigger = clock_time(); \
} while(0)
/*---------------------------------------------------------------------------*/
/* Sequence Values and Serial Number Arithmetic
 *
 * Sequence Number Comparisons as per RFC1982 "Serial Number Arithmetic"
 * Our 'SERIAL_BITS' value is 15 here
 *
 * NOTE: There can be pairs of sequence numbers s1 and s2 with an undefined
 * ordering. All three macros would evaluate as 0, as in:
 * SEQ_VAL_IS_EQUAL(s1, s2) == 0 and
 * SEQ_VAL_IS_GT(s1, s2)    == 0 and
 * SEQ_VAL_IS_LT(s1, s2)    == 0
 *
 * This is not a bug of this implementation, it's an RFC design choice
 */

/**
 * \brief s1 is said to be equal s2 iif SEQ_VAL_IS_EQ(s1, s2) == 1
 */
#define SEQ_VAL_IS_EQ(i1, i2) ((i1) == (i2))

/**
 * \brief s1 is said to be less than s2 iif SEQ_VAL_IS_LT(s1, s2) == 1
 */
#define SEQ_VAL_IS_LT(i1, i2) \
  ( \
    ((i1) != (i2)) && \
    ((((i1) < (i2)) && ((int16_t)((i2) - (i1)) < 0x4000)) || \
     (((i1) > (i2)) && ((int16_t)((i1) - (i2)) > 0x4000))) \
  )

/**
 * \brief s1 is said to be greater than s2 iif SEQ_VAL_IS_LT(s1, s2) == 1
 */
#define SEQ_VAL_IS_GT(i1, i2) \
( \
  ((i1) != (i2)) && \
  ((((i1) < (i2)) && ((int16_t)((i2) - (i1)) > 0x4000)) || \
   (((i1) > (i2)) && ((int16_t)((i1) - (i2)) < 0x4000))) \
)

/**
 * \brief Add n to s: (s + n) modulo (2 ^ SERIAL_BITS) => ((s + n) % 0x8000)
 */
#define SEQ_VAL_ADD(s, n) (((s) + (n)) % 0x8000)
/*---------------------------------------------------------------------------*/
/* Sliding Windows */
struct sliding_window {
  seed_id_t seed_id;
  int16_t lower_bound;          /* lolipop */
  int16_t upper_bound;          /* lolipop */
  int16_t min_listed;           /* lolipop */
  uint8_t flags;                /* Is used, Trickle param, Is listed */
  uint8_t count;
};

#define SLIDING_WINDOW_U_BIT 0x80       /* Is used */
#define SLIDING_WINDOW_M_BIT 0x40       /* Window trickle parametrization */
#define SLIDING_WINDOW_L_BIT 0x20       /* Current ICMP message lists us */
#define SLIDING_WINDOW_B_BIT 0x10       /* Used when updating bounds */

/**
 * \brief Is Occupied sliding window location w
 * w: pointer to a sliding window
 */
#define SLIDING_WINDOW_IS_USED(w) ((w)->flags & SLIDING_WINDOW_U_BIT)

/**
 * \brief Set 'Is Used' bit for window w
 * w: pointer to a sliding window
 */
#define SLIDING_WINDOW_IS_USED_SET(w) ((w)->flags |= SLIDING_WINDOW_U_BIT)

/**
 * \brief Clear 'Is Used' bit for window w
 * w: pointer to a sliding window
 */
#define SLIDING_WINDOW_IS_USED_CLR(w) ((w)->flags &= ~SLIDING_WINDOW_U_BIT)
#define window_free(w) SLIDING_WINDOW_IS_USED_CLR(w)

/**
 * \brief Set 'Is Seen' bit for window w
 * w: pointer to a sliding window
 */
#define SLIDING_WINDOW_LISTED_SET(w) ((w)->flags |= SLIDING_WINDOW_L_BIT)

/**
 * \brief Clear 'Is Seen' bit for window w
 * w: pointer to a sliding window
 */
#define SLIDING_WINDOW_LISTED_CLR(w) ((w)->flags &= ~SLIDING_WINDOW_L_BIT)

/**
 * \brief Is the sliding window at location w listed in current ICMP message?
 * w: pointer to a sliding window
 */
#define SLIDING_WINDOW_IS_LISTED(w) ((w)->flags & SLIDING_WINDOW_L_BIT)

/**
 * \brief Set M bit for window w
 * w: pointer to a sliding window
 */
#define SLIDING_WINDOW_M_SET(w) ((w)->flags |= SLIDING_WINDOW_M_BIT)

/**
 * \brief Clear M bit for window w
 * w: pointer to a sliding window
 */
#define SLIDING_WINDOW_M_CLR(w) ((w)->flags &= ~SLIDING_WINDOW_M_BIT)

/**
 * \brief Retrieve trickle parametrization for sliding window at location w
 * w: pointer to a sliding window
 */
#define SLIDING_WINDOW_GET_M(w) \
  ((uint8_t)(((w)->flags & SLIDING_WINDOW_M_BIT) == SLIDING_WINDOW_M_BIT))
/*---------------------------------------------------------------------------*/
/* Multicast Packet Buffers */
struct mcast_packet {
#if ROLL_TM_SHORT_SEEDS
  /* Short seeds are stored inside the message */
  seed_id_t seed_id;
#endif
  uint32_t active;              /* Starts at 0 and increments */
  uint32_t dwell;               /* Starts at 0 and increments */
  uint16_t buff_len;
  uint16_t seq_val;             /* host-byte order */
  struct sliding_window *sw;    /* Pointer to the SW this packet belongs to */
  uint8_t flags;                /* Is-Used, Must Send, Is Listed */
  uint8_t buff[UIP_BUFSIZE - UIP_LLH_LEN];
};

/* Flag bits */
#define MCAST_PACKET_U_BIT       0x80   /* Is Used */
#define MCAST_PACKET_S_BIT       0x20   /* Must Send Next Pass */
#define MCAST_PACKET_L_BIT       0x10   /* Is listed in ICMP message */

/* Fetch a pointer to the Seed ID of a buffered message p */
#if ROLL_TM_SHORT_SEEDS
#define MCAST_PACKET_GET_SEED(p) ((seed_id_t *)&((p)->seed_id))
#else
#define MCAST_PACKET_GET_SEED(p) \
    ((seed_id_t *)&((struct uip_ip_hdr *)&(p)->buff[UIP_LLH_LEN])->srcipaddr)
#endif

/**
 * \brief Get the TTL of a buffered packet
 * p: pointer to a packet buffer
 */
#define MCAST_PACKET_TTL(p) \
    (((struct uip_ip_hdr *)(p)->buff)->ttl)

/**
 * \brief Set 'Is Used' bit for packet p
 * p: pointer to a packet buffer
 */
#define MCAST_PACKET_USED_SET(p) ((p)->flags |= MCAST_PACKET_U_BIT)

/**
 * \brief Clear 'Is Used' bit for packet p
 * p: pointer to a packet buffer
 */
#define MCAST_PACKET_USED_CLR(p) ((p)->flags &= ~MCAST_PACKET_U_BIT)

/**
 * \brief Is Occupied buffer location p
 */
#define MCAST_PACKET_IS_USED(p) ((p)->flags & MCAST_PACKET_U_BIT)

/**
 * \brief Must we send this message this pass?
 */
#define MCAST_PACKET_MUST_SEND(p) ((p)->flags & MCAST_PACKET_S_BIT)

/**
 * \brief Set 'Must Send' bit for message p
 * p: pointer to a struct mcast_packet
 */
#define MCAST_PACKET_SEND_SET(p) ((p)->flags |= MCAST_PACKET_S_BIT)

/**
 * \brief Clear 'Must Send' bit for message p
 * p: pointer to a struct mcast_packet
 */
#define MCAST_PACKET_SEND_CLR(p) ((p)->flags &= ~MCAST_PACKET_S_BIT)

/**
 * \brief Is the message p listed in current ICMP message?
 * p: pointer to a struct mcast_packet
 */
#define MCAST_PACKET_IS_LISTED(p) ((p)->flags & MCAST_PACKET_L_BIT)

/**
 * \brief Set 'Is Listed' bit for message p
 * p: pointer to a struct mcast_packet
 */
#define MCAST_PACKET_LISTED_SET(p) ((p)->flags |= MCAST_PACKET_L_BIT)

/**
 * \brief Clear 'Is Listed' bit for message p
 * p: pointer to a struct mcast_packet
 */
#define MCAST_PACKET_LISTED_CLR(p) ((p)->flags &= ~MCAST_PACKET_L_BIT)

/**
 * \brief Free a multicast packet buffer
 * p: pointer to a struct mcast_packet
 */
#define MCAST_PACKET_FREE(p) ((p)->flags = 0)
/*---------------------------------------------------------------------------*/
/* Sequence Lists in Multicast Trickle ICMP messages */
struct sequence_list_header {
  uint8_t flags;                /* S: Seed ID length, M: Trickle parametrization */
  uint8_t seq_len;
  seed_id_t seed_id;
};

#define SEQUENCE_LIST_S_BIT 0x80
#define SEQUENCE_LIST_M_BIT 0x40
#define SEQUENCE_LIST_RES   0x3F

/**
 * \brief Get the Trickle Parametrization for an ICMPv6 sequence list
 * l: pointer to a sequence list structure
 */
#define SEQUENCE_LIST_GET_M(l) \
  ((uint8_t)(((l)->flags & SEQUENCE_LIST_M_BIT) == SEQUENCE_LIST_M_BIT))

/**
 * \brief Get the Seed ID Length for an ICMPv6 sequence list
 * l: pointer to a sequence list structure
 */
#define SEQUENCE_LIST_GET_S(l) \
    ((uint8_t)(((l)->flags & SEQUENCE_LIST_S_BIT) == SEQUENCE_LIST_S_BIT))
/*---------------------------------------------------------------------------*/
/* Trickle Multicast HBH Option */
struct hbho_mcast {
  uint8_t type;
  uint8_t len;
#if ROLL_TM_SHORT_SEEDS
  seed_id_t seed_id;
#endif
  uint8_t flags;                /* M, Seq ID MSB */
  uint8_t seq_id_lsb;
#if !ROLL_TM_SHORT_SEEDS
  /* Need to Pad to 8 bytes with PadN */
  uint8_t padn_type;            /* 1: PadN */
  uint8_t padn_len;             /* 0->2 bytes */
#endif
};

#define HBHO_OPT_TYPE_TRICKLE 0x0C
#define HBHO_LEN_LONG_SEED       2
#define HBHO_LEN_SHORT_SEED      4
#define HBHO_TOTAL_LEN           8
/**
 * \brief Get the Trickle Parametrization for a multicast HBHO header
 * m: pointer to the HBHO header
 */
#define HBH_GET_M(h) (((h)->flags & 0x80) == 0x80)

/**
 * \brief Set the Trickle Parametrization bit for a multicast HBHO header
 * m: pointer to the HBHO header
 */
#define HBH_SET_M(h) ((h)->flags |= 0x80)

/**
 * \brief Retrieve the Sequence Value MSB from a multicast HBHO header
 * m: pointer to the HBHO header
 */
#define HBH_GET_SV_MSB(h) ((h)->flags & 0x7F)
/*---------------------------------------------------------------------------*/
/* Destination for our ICMPv6 datagrams */
#if ROLL_TM_CONF_DEST_ALL_NODES
#define roll_tm_create_dest(a) uip_create_linklocal_allnodes_mcast(a)
#else
#define roll_tm_create_dest(a) uip_create_linklocal_allrouters_mcast(a)
#endif
/*---------------------------------------------------------------------------*/
/* Maintain Stats */
#if UIP_MCAST6_STATS
static struct roll_tm_stats stats;

#define ROLL_TM_STATS_ADD(x) stats.x++
#define ROLL_TM_STATS_INIT() do { memset(&stats, 0, sizeof(stats)); } while(0)
#else /* UIP_MCAST6_STATS */
#define ROLL_TM_STATS_ADD(x)
#define ROLL_TM_STATS_INIT()
#endif
/*---------------------------------------------------------------------------*/
/* Internal Data Structures */
/*---------------------------------------------------------------------------*/
static struct trickle_param t[2];
static struct sliding_window windows[ROLL_TM_WINS];
static struct mcast_packet buffered_msgs[ROLL_TM_BUFF_NUM];
/*---------------------------------------------------------------------------*/
/* Temporary Stores */
/*---------------------------------------------------------------------------*/
static struct trickle_param *loctpptr;
static struct sequence_list_header *locslhptr;
static struct sliding_window *locswptr;
static struct sliding_window *iterswptr;
static struct mcast_packet *locmpptr;
static struct hbho_mcast *lochbhmptr;
static uint16_t last_seq;
/*---------------------------------------------------------------------------*/
/* uIPv6 Pointers */
/*---------------------------------------------------------------------------*/
#define UIP_DATA_BUF      ((uint8_t *)&uip_buf[uip_l2_l3_hdr_len + UIP_UDPH_LEN])
#define UIP_UDP_BUF       ((struct uip_udp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_EXT_BUF       ((struct uip_ext_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN])
#define UIP_EXT_BUF_NEXT  ((uint8_t *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN + HBHO_TOTAL_LEN])
#define UIP_EXT_OPT_FIRST ((struct hbho_mcast *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN + 2])
#define UIP_IP_BUF        ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_ICMP_BUF      ((struct uip_icmp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_ICMP_PAYLOAD  ((unsigned char *)&uip_buf[uip_l2_l3_icmp_hdr_len])
extern uint16_t uip_slen;
/*---------------------------------------------------------------------------*/
/* Local function prototypes */
/*---------------------------------------------------------------------------*/
static void icmp_input(void);
static void icmp_output(void);
static void window_update_bounds(void);
static void reset_trickle_timer(uint8_t);
static void handle_timer(void *);
/*---------------------------------------------------------------------------*/
/* ROLL TM ICMPv6 handler declaration */
UIP_ICMP6_HANDLER(roll_tm_icmp_handler, ICMP6_ROLL_TM,
                  UIP_ICMP6_HANDLER_CODE_ANY, icmp_input);
/*---------------------------------------------------------------------------*/
/* Return a random number in [I/2, I), for a timer with Imin when the timer's
 * current number of doublings is d */
static clock_time_t
random_interval(clock_time_t i_min, uint8_t d)
{
  clock_time_t min = TRICKLE_TIME(i_min >> 1, d);

  VERBOSE_PRINTF("ROLL TM: Random [%lu, %lu)\n", (unsigned long)min,
                 (unsigned long)(TRICKLE_TIME(i_min, d)));

  return min + (random_rand() % (TRICKLE_TIME(i_min, d) - 1 - min));
}
/*---------------------------------------------------------------------------*/
/* Called at the end of the current interval for timer ptr */
static void
double_interval(void *ptr)
{
  struct trickle_param *param = (struct trickle_param *)ptr;
  int16_t offset;
  clock_time_t next;

  /*
   * If we got called long past our expiration, store the offset and try to
   * compensate this period
   */
  offset = (int16_t)(clock_time() - param->t_end);

  /* Calculate next interval */
  if(param->i_current < param->i_max) {
    param->i_current++;
  }

  param->t_start = param->t_end;
  param->t_end = param->t_start + (param->i_min << param->i_current);

  next = random_interval(param->i_min, param->i_current);
  if(next > offset) {
    next -= offset;
  } else {
    next = 0;
  }
  param->t_next = next;
  ctimer_set(&param->ct, param->t_next, handle_timer, (void *)param);

  VERBOSE_PRINTF("ROLL TM: Doubling at %lu (offset %d), Start %lu, End %lu,"
                 " Periodic in %lu\n", clock_time(), offset,
                 (unsigned long)param->t_start,
                 (unsigned long)param->t_end, (unsigned long)param->t_next);
}
/*---------------------------------------------------------------------------*/
/*
 * Called at a random point in [I/2,I) of the current interval for ptr
 * PARAM is a pointer to the timer that triggered the callback (&t[index])
 */
static void
handle_timer(void *ptr)
{
  struct trickle_param *param;
  clock_time_t diff_last;       /* Time diff from last pass */
  clock_time_t diff_start;      /* Time diff from interval start */
  uint8_t m;

  param = (struct trickle_param *)ptr;
  if(param == &t[0]) {
    m = 0;
  } else if(param == &t[1]) {
    m = 1;
  } else {
    /* This is an ooops and a serious one too */
    return;
  }

  /* Bail out pronto if our uIPv6 stack is not ready to send messages */
  if(uip_ds6_get_link_local(ADDR_PREFERRED) == NULL) {
    VERBOSE_PRINTF
      ("ROLL TM: Suppressing timer processing. Stack not ready\n");
    reset_trickle_timer(m);
    return;
  }

  VERBOSE_PRINTF("ROLL TM: M=%u Periodic at %lu, last=%lu\n",
                 m, (unsigned long)clock_time(),
                 (unsigned long)param->t_last_trigger);

  /* Temporarily store 'now' in t_next and calculate diffs */
  param->t_next = clock_time();
  diff_last = param->t_next - param->t_last_trigger;
  diff_start = param->t_next - param->t_start;
  param->t_last_trigger = param->t_next;

  VERBOSE_PRINTF
    ("ROLL TM: M=%u Periodic diff from last %lu, from start %lu\n", m,
     (unsigned long)diff_last, (unsigned long)diff_start);

  /* Handle all buffered messages */
  for(locmpptr = &buffered_msgs[ROLL_TM_BUFF_NUM - 1];
      locmpptr >= buffered_msgs; locmpptr--) {
    if(MCAST_PACKET_IS_USED(locmpptr)
       && (SLIDING_WINDOW_GET_M(locmpptr->sw) == m)) {

      /*
       * if()
       * If the packet was received during the last interval, its reception
       * caused an inconsistency (and thus a timer reset). This means that
       * the packet was received at about t_start, we increment by diff_start
       *
       * else()
       * If the packet was not received during the last window, it is safe to
       * increase its lifetime counters by the time diff from last pass
       *
       * if active == dwell == 0 but i_current != 0, this is an oops
       * (new packet that didn't reset us). We don't handle it
       */
      if(locmpptr->active == 0) {
        locmpptr->active += diff_start;
        locmpptr->dwell += diff_start;
      } else {
        locmpptr->active += diff_last;
        locmpptr->dwell += diff_last;
      }

      VERBOSE_PRINTF("ROLL TM: M=%u Packet %u active %lu of %lu\n",
                     m, locmpptr->seq_val, locmpptr->active,
                     TRICKLE_ACTIVE(param));

      if(locmpptr->dwell > TRICKLE_DWELL(param)) {
        locmpptr->sw->count--;
        PRINTF("ROLL TM: M=%u Free Packet %u (%lu > %lu), Window now at %u\n",
               m, locmpptr->seq_val, locmpptr->dwell,
               TRICKLE_DWELL(param), locmpptr->sw->count);
        if(locmpptr->sw->count == 0) {
          PRINTF("ROLL TM: M=%u Free Window ", m);
          PRINT_SEED(&locmpptr->sw->seed_id);
          PRINTF("\n");
          window_free(locmpptr->sw);
        }
        MCAST_PACKET_FREE(locmpptr);
      } else if(MCAST_PACKET_TTL(locmpptr) > 0) {
        /* Handle multicast transmissions */
        if(locmpptr->active < TRICKLE_ACTIVE(param) &&
           ((SUPPRESSION_ENABLED(param) && MCAST_PACKET_MUST_SEND(locmpptr)) ||
           SUPPRESSION_DISABLED(param))) {
          PRINTF("ROLL TM: M=%u Periodic - Sending packet from Seed ", m);
          PRINT_SEED(&locmpptr->sw->seed_id);
          PRINTF(" seq %u\n", locmpptr->seq_val);
          uip_len = locmpptr->buff_len;
          memcpy(UIP_IP_BUF, &locmpptr->buff, uip_len);

          UIP_MCAST6_STATS_ADD(mcast_fwd);
          tcpip_output(NULL);
          MCAST_PACKET_SEND_CLR(locmpptr);
          watchdog_periodic();
        }
      }
    }
  }

  /* Suppression Enabled - Send an ICMP */
  if(SUPPRESSION_ENABLED(param)) {
    if(param->c < param->k) {
      icmp_output();
    }
  }

  /* Done handling inconsistencies for this timer */
  param->inconsistency = 0;
  param->c = 0;

  window_update_bounds();

  /* Temporarily store 'now' in t_next */
  param->t_next = clock_time();
  if(param->t_next >= param->t_end) {
    /* took us too long to process things, double interval asap */
    param->t_next = 0;
  } else {
    param->t_next = param->t_end - param->t_next;
  }
  VERBOSE_PRINTF
    ("ROLL TM: M=%u Periodic at %lu, Interval End at %lu in %lu\n", m,
     (unsigned long)clock_time(), (unsigned long)param->t_end,
     (unsigned long)param->t_next);
  ctimer_set(&param->ct, param->t_next, double_interval, (void *)param);

  return;
}
/*---------------------------------------------------------------------------*/
static void
reset_trickle_timer(uint8_t index)
{
  t[index].t_start = clock_time();
  t[index].t_end = t[index].t_start + (t[index].i_min);
  t[index].i_current = 0;
  t[index].c = 0;
  t[index].t_next = random_interval(t[index].i_min, t[index].i_current);

  VERBOSE_PRINTF
    ("ROLL TM: M=%u Reset at %lu, Start %lu, End %lu, New Interval %lu\n",
     index, (unsigned long)t[index].t_start, (unsigned long)t[index].t_start,
     (unsigned long)t[index].t_end, (unsigned long)t[index].t_next);

  ctimer_set(&t[index].ct, t[index].t_next, handle_timer, (void *)&t[index]);
}
/*---------------------------------------------------------------------------*/
static struct sliding_window *
window_allocate()
{
  for(iterswptr = &windows[ROLL_TM_WINS - 1]; iterswptr >= windows;
      iterswptr--) {
    if(!SLIDING_WINDOW_IS_USED(iterswptr)) {
      iterswptr->count = 0;
      iterswptr->lower_bound = -1;
      iterswptr->upper_bound = -1;
      iterswptr->min_listed = -1;
      return iterswptr;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static struct sliding_window *
window_lookup(seed_id_t *s, uint8_t m)
{
  for(iterswptr = &windows[ROLL_TM_WINS - 1]; iterswptr >= windows;
      iterswptr--) {
    VERBOSE_PRINTF("ROLL TM: M=%u (%u) ", SLIDING_WINDOW_GET_M(iterswptr), m);
    VERBOSE_PRINT_SEED(&iterswptr->seed_id);
    VERBOSE_PRINTF("\n");
    if(seed_id_cmp(s, &iterswptr->seed_id) &&
       SLIDING_WINDOW_GET_M(iterswptr) == m) {
      return iterswptr;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static void
window_update_bounds()
{
  for(iterswptr = &windows[ROLL_TM_WINS - 1]; iterswptr >= windows;
      iterswptr--) {
    iterswptr->lower_bound = -1;
  }

  for(locmpptr = &buffered_msgs[ROLL_TM_BUFF_NUM - 1];
      locmpptr >= buffered_msgs; locmpptr--) {
    if(MCAST_PACKET_IS_USED(locmpptr)) {
      iterswptr = locmpptr->sw;
      VERBOSE_PRINTF("ROLL TM: Update Bounds: [%d - %d] vs %u\n",
                     iterswptr->lower_bound, iterswptr->upper_bound,
                     locmpptr->seq_val);
      if(iterswptr->lower_bound < 0
         || SEQ_VAL_IS_LT(locmpptr->seq_val, iterswptr->lower_bound)) {
        iterswptr->lower_bound = locmpptr->seq_val;
      }
      if(iterswptr->upper_bound < 0 ||
         SEQ_VAL_IS_GT(locmpptr->seq_val, iterswptr->upper_bound)) {
        iterswptr->upper_bound = locmpptr->seq_val;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static struct mcast_packet *
buffer_reclaim()
{
  struct sliding_window *largest = windows;
  struct mcast_packet *rv;

  for(iterswptr = &windows[ROLL_TM_WINS - 1]; iterswptr >= windows;
      iterswptr--) {
    if(iterswptr->count > largest->count) {
      largest = iterswptr;
    }
  }

  if(largest->count == 1) {
    /* Can't reclaim last entry for a window and this is the largest window */
    return NULL;
  }

  PRINTF("ROLL TM: Reclaim from Seed ");
  PRINT_SEED(&largest->seed_id);
  PRINTF(" M=%u, count was %u\n",
         SLIDING_WINDOW_GET_M(largest), largest->count);
  /* Find the packet at the lowest bound for the largest window */
  for(locmpptr = &buffered_msgs[ROLL_TM_BUFF_NUM - 1];
      locmpptr >= buffered_msgs; locmpptr--) {
    if(MCAST_PACKET_IS_USED(locmpptr) && (locmpptr->sw == largest) &&
       SEQ_VAL_IS_EQ(locmpptr->seq_val, largest->lower_bound)) {
      rv = locmpptr;
      PRINTF("ROLL TM: Reclaim seq. val %u\n", locmpptr->seq_val);
      MCAST_PACKET_FREE(rv);
      largest->count--;
      window_update_bounds();
      VERBOSE_PRINTF("ROLL TM: Reclaim - new bounds [%u , %u]\n",
                     largest->lower_bound, largest->upper_bound);
      return rv;
    }
  }

  /* oops */
  return NULL;
}
/*---------------------------------------------------------------------------*/
static struct mcast_packet *
buffer_allocate()
{
  for(locmpptr = &buffered_msgs[ROLL_TM_BUFF_NUM - 1];
      locmpptr >= buffered_msgs; locmpptr--) {
    if(!MCAST_PACKET_IS_USED(locmpptr)) {
      return locmpptr;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static void
icmp_output()
{
  struct sequence_list_header *sl;
  uint8_t *buffer;
  uint16_t payload_len;

  PRINTF("ROLL TM: ICMPv6 Out\n");

  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 0;
  UIP_IP_BUF->flow = 0;
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = ROLL_TM_IP_HOP_LIMIT;

  sl = (struct sequence_list_header *)UIP_ICMP_PAYLOAD;
  payload_len = 0;

  VERBOSE_PRINTF("ROLL TM: ICMPv6 Out - Hdr @ %p, payload @ %p\n", UIP_ICMP_BUF, sl);

  for(iterswptr = &windows[ROLL_TM_WINS - 1]; iterswptr >= windows;
      iterswptr--) {
    if(SLIDING_WINDOW_IS_USED(iterswptr) && iterswptr->count > 0) {
      memset(sl, 0, sizeof(struct sequence_list_header));
#if ROLL_TM_SHORT_SEEDS
      sl->flags = SEQUENCE_LIST_S_BIT;
#endif
      if(SLIDING_WINDOW_GET_M(iterswptr)) {
        sl->flags |= SEQUENCE_LIST_M_BIT;
      }
      seed_id_cpy(&sl->seed_id, &iterswptr->seed_id);

      PRINTF("ROLL TM: ICMPv6 Out - Seq. F=0x%02x, Seed ID=", sl->flags);
      PRINT_SEED(&sl->seed_id);

      buffer = (uint8_t *)sl + sizeof(struct sequence_list_header);

      for(locmpptr = &buffered_msgs[ROLL_TM_BUFF_NUM - 1];
          locmpptr >= buffered_msgs; locmpptr--) {
        if(MCAST_PACKET_IS_USED(locmpptr) &&
           locmpptr->active < TRICKLE_ACTIVE((&t[SLIDING_WINDOW_GET_M(iterswptr)]))) {
          if(locmpptr->sw == iterswptr) {
            sl->seq_len++;
            PRINTF(", %u", locmpptr->seq_val);
            *buffer = (uint8_t)(locmpptr->seq_val >> 8);
            buffer++;
            *buffer = (uint8_t)(locmpptr->seq_val & 0xFF);
            buffer++;
          }
        }
      }
      PRINTF(", Len=%u\n", sl->seq_len);

      /* Scrap the entire window if it has no content */
      if(sl->seq_len > 0) {
        payload_len += sizeof(struct sequence_list_header) + sl->seq_len * 2;
        sl = (struct sequence_list_header *)buffer;
      }
    }
  }

  if(payload_len == 0) {
    VERBOSE_PRINTF("ROLL TM: ICMPv6 Out - nothing to send\n");
    return;
  }

  roll_tm_create_dest(&UIP_IP_BUF->destipaddr);
  uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);

  UIP_IP_BUF->len[0] = (UIP_ICMPH_LEN + payload_len) >> 8;
  UIP_IP_BUF->len[1] = (UIP_ICMPH_LEN + payload_len) & 0xff;

  UIP_ICMP_BUF->type = ICMP6_ROLL_TM;
  UIP_ICMP_BUF->icode = ROLL_TM_ICMP_CODE;

  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();

  uip_len = UIP_IPH_LEN + UIP_ICMPH_LEN + payload_len;

  VERBOSE_PRINTF("ROLL TM: ICMPv6 Out - %u bytes\n", payload_len);

  tcpip_ipv6_output();
  ROLL_TM_STATS_ADD(icmp_out);
  return;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Processes an incoming or outgoing multicast message and determines
 * whether it should be dropped or accepted
 *
 * \param in 1: Incoming packet, 0: Outgoing (we are the seed)
 *
 * \return 0: Drop, 1: Accept
 */
static uint8_t
accept(uint8_t in)
{
  seed_id_t *seed_ptr;
  uint8_t m;
  uint16_t seq_val;

  PRINTF("ROLL TM: Multicast I/O\n");

#if UIP_CONF_IPV6_CHECKS
  if(uip_is_addr_mcast_non_routable(&UIP_IP_BUF->destipaddr)) {
    PRINTF("ROLL TM: Mcast I/O, bad destination\n");
    UIP_MCAST6_STATS_ADD(mcast_bad);
    return UIP_MCAST6_DROP;
  }
  /*
   * Abort transmission if the v6 src is unspecified. This may happen if the
   * seed tries to TX while it's still performing DAD or waiting for a prefix
   */
  if(uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr)) {
    PRINTF("ROLL TM: Mcast I/O, bad source\n");
    UIP_MCAST6_STATS_ADD(mcast_bad);
    return UIP_MCAST6_DROP;
  }
#endif

  /* Check the Next Header field: Must be HBHO */
  if(UIP_IP_BUF->proto != UIP_PROTO_HBHO) {
    PRINTF("ROLL TM: Mcast I/O, bad proto\n");
    UIP_MCAST6_STATS_ADD(mcast_bad);
    return UIP_MCAST6_DROP;
  } else {
    /* Check the Option Type */
    if(UIP_EXT_OPT_FIRST->type != HBHO_OPT_TYPE_TRICKLE) {
      PRINTF("ROLL TM: Mcast I/O, bad HBHO type\n");
      UIP_MCAST6_STATS_ADD(mcast_bad);
      return UIP_MCAST6_DROP;
    }
  }
  lochbhmptr = UIP_EXT_OPT_FIRST;

  PRINTF("ROLL TM: HBHO T=%u, L=%u, M=%u, S=0x%02x%02x\n",
         lochbhmptr->type, lochbhmptr->len, HBH_GET_M(lochbhmptr),
         HBH_GET_SV_MSB(lochbhmptr), lochbhmptr->seq_id_lsb);

  /* Drop unsupported Seed ID Lengths. S bit: 0->short, 1->long */
#if ROLL_TM_SHORT_SEEDS
  /* Short Seed ID: Len MUST be 4 */
  if(lochbhmptr->len != HBHO_LEN_SHORT_SEED) {
    PRINTF("ROLL TM: Mcast I/O, bad length\n");
    UIP_MCAST6_STATS_ADD(mcast_bad);
    return UIP_MCAST6_DROP;
  }
#else
  /* Long Seed ID: Len MUST be 2 (Seed ID is elided) */
  if(lochbhmptr->len != HBHO_LEN_LONG_SEED) {
    PRINTF("ROLL TM: Mcast I/O, bad length\n");
    UIP_MCAST6_STATS_ADD(mcast_bad);
    return UIP_MCAST6_DROP;
  }
#endif

#if UIP_MCAST6_STATS
  if(in == ROLL_TM_DGRAM_IN) {
    UIP_MCAST6_STATS_ADD(mcast_in_all);
  }
#endif

  /* Is this for a known window? */
#if ROLL_TM_SHORT_SEEDS
  seed_ptr = &lochbhmptr->seed_id;
#else
  seed_ptr = &UIP_IP_BUF->srcipaddr;
#endif
  m = HBH_GET_M(lochbhmptr);

  locswptr = window_lookup(seed_ptr, m);

  seq_val = lochbhmptr->seq_id_lsb;
  seq_val |= HBH_GET_SV_MSB(lochbhmptr) << 8;

  if(locswptr) {
    if(SEQ_VAL_IS_LT(seq_val, locswptr->lower_bound)) {
      /* Too old, drop */
      PRINTF("ROLL TM: Too old\n");
      UIP_MCAST6_STATS_ADD(mcast_dropped);
      return UIP_MCAST6_DROP;
    }
    for(locmpptr = &buffered_msgs[ROLL_TM_BUFF_NUM - 1];
        locmpptr >= buffered_msgs; locmpptr--) {
      if(MCAST_PACKET_IS_USED(locmpptr) &&
         locmpptr->sw == locswptr &&
         SLIDING_WINDOW_GET_M(locmpptr->sw) == m &&
         SEQ_VAL_IS_EQ(seq_val, locmpptr->seq_val)) {
        /* Seen before , drop */
        PRINTF("ROLL TM: Seen before\n");
        UIP_MCAST6_STATS_ADD(mcast_dropped);
        return UIP_MCAST6_DROP;
      }
    }
  }

  PRINTF("ROLL TM: New message\n");

  /* We have not seen this message before */
  /* Allocate a window if we have to */
  if(!locswptr) {
    locswptr = window_allocate();
    PRINTF("ROLL TM: New seed\n");
  }
  if(!locswptr) {
    /* Couldn't allocate window, drop */
    PRINTF("ROLL TM: Failed to allocate window\n");
    UIP_MCAST6_STATS_ADD(mcast_dropped);
    return UIP_MCAST6_DROP;
  }

  /* Allocate a buffer */
  locmpptr = buffer_allocate();
  if(!locmpptr) {
    PRINTF("ROLL TM: Buffer allocation failed, reclaiming\n");
    locmpptr = buffer_reclaim();
  }

  if(!locmpptr) {
    /* Failed to allocate / reclaim a buffer. If the window has only just been
     * allocated, free it before dropping */
    PRINTF("ROLL TM: Buffer reclaim failed\n");
    if(locswptr->count == 0) {
      window_free(locswptr);
      UIP_MCAST6_STATS_ADD(mcast_dropped);
      return UIP_MCAST6_DROP;
    }
  }
#if UIP_MCAST6_STATS
  if(in == ROLL_TM_DGRAM_IN) {
    UIP_MCAST6_STATS_ADD(mcast_in_unique);
  }
#endif

  /* We have a window and we have a buffer. Accept this message */
  /* Set the seed ID and correct M for this window */
  SLIDING_WINDOW_M_CLR(locswptr);
  if(m) {
    SLIDING_WINDOW_M_SET(locswptr);
  }
  SLIDING_WINDOW_IS_USED_SET(locswptr);
  seed_id_cpy(&locswptr->seed_id, seed_ptr);
  PRINTF("ROLL TM: Window for seed ");
  PRINT_SEED(&locswptr->seed_id);
  PRINTF(" M=%u, count=%u\n",
         SLIDING_WINDOW_GET_M(locswptr), locswptr->count);

  /* If this window was previously empty, set its lower bound to this packet */
  if(locswptr->count == 0) {
    locswptr->lower_bound = seq_val;
    VERBOSE_PRINTF("ROLL TM: New Lower Bound %u\n", locswptr->lower_bound);
  }

  /* If this is a new Seq Num, update the window upper bound */
  if(locswptr->count == 0 || SEQ_VAL_IS_GT(seq_val, locswptr->upper_bound)) {
    locswptr->upper_bound = seq_val;
    VERBOSE_PRINTF("ROLL TM: New Upper Bound %u\n", locswptr->upper_bound);
  }

  locswptr->count++;

  memset(locmpptr, 0, sizeof(struct mcast_packet));
  memcpy(&locmpptr->buff, UIP_IP_BUF, uip_len);
  locmpptr->sw = locswptr;
  locmpptr->buff_len = uip_len;
  locmpptr->seq_val = seq_val;
  MCAST_PACKET_USED_SET(locmpptr);

  PRINTF("ROLL TM: Window for seed ");
  PRINT_SEED(&locswptr->seed_id);
  PRINTF(" M=%u, %u values within [%u , %u]\n",
         SLIDING_WINDOW_GET_M(locswptr), locswptr->count,
         locswptr->lower_bound, locswptr->upper_bound);

  /*
   * If this is an incoming packet, it is inconsistent and we need to decrement
   * its TTL before we start forwarding it.
   * If on the other hand we are the seed, the caller will trigger a
   * transmission so we don't flag inconsistency and we leave the TTL alone
   */
  if(in == ROLL_TM_DGRAM_IN) {
    MCAST_PACKET_SEND_SET(locmpptr);
    MCAST_PACKET_TTL(locmpptr)--;

    t[m].inconsistency = 1;

    PRINTF("ROLL TM: Inconsistency. Reset T%u\n", m);
    reset_trickle_timer(m);
  }

  /* Deliver if necessary */
  return UIP_MCAST6_ACCEPT;
}
/*---------------------------------------------------------------------------*/
/* ROLL TM ICMPv6 Input Handler */
static void
icmp_input()
{
  uint8_t inconsistency;
  uint16_t *seq_ptr;
  uint16_t *end_ptr;
  uint16_t val;

#if UIP_CONF_IPV6_CHECKS
  if(!uip_is_addr_link_local(&UIP_IP_BUF->srcipaddr)) {
    PRINTF("ROLL TM: ICMPv6 In, bad source ");
    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
    PRINTF(" to ");
    PRINT6ADDR(&UIP_IP_BUF->destipaddr);
    PRINTF("\n");
    ROLL_TM_STATS_ADD(icmp_bad);
    return;
  }

  if(!uip_is_addr_linklocal_allnodes_mcast(&UIP_IP_BUF->destipaddr)
     && !uip_is_addr_linklocal_allrouters_mcast(&UIP_IP_BUF->destipaddr)) {
    PRINTF("ROLL TM: ICMPv6 In, bad destination\n");
    ROLL_TM_STATS_ADD(icmp_bad);
    return;
  }

  if(UIP_ICMP_BUF->icode != ROLL_TM_ICMP_CODE) {
    PRINTF("ROLL TM: ICMPv6 In, bad ICMP code\n");
    ROLL_TM_STATS_ADD(icmp_bad);
    return;
  }

  if(UIP_IP_BUF->ttl != ROLL_TM_IP_HOP_LIMIT) {
    PRINTF("ROLL TM: ICMPv6 In, bad TTL\n");
    ROLL_TM_STATS_ADD(icmp_bad);
    return;
  }
#endif

  PRINTF("ROLL TM: ICMPv6 In from ");
  PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
  PRINTF(" len %u, ext %u\n", uip_len, uip_ext_len);

  ROLL_TM_STATS_ADD(icmp_in);

  /* Reset Is-Listed bit for all windows */
  for(iterswptr = &windows[ROLL_TM_WINS - 1]; iterswptr >= windows;
      iterswptr--) {
    SLIDING_WINDOW_LISTED_CLR(iterswptr);
  }

  /* Reset Is-Listed bit for all cached packets */
  for(locmpptr = &buffered_msgs[ROLL_TM_BUFF_NUM - 1];
      locmpptr >= buffered_msgs; locmpptr--) {
    MCAST_PACKET_LISTED_CLR(locmpptr);
  }

  locslhptr = (struct sequence_list_header *)UIP_ICMP_PAYLOAD;

  VERBOSE_PRINTF("ROLL TM: ICMPv6 In, parse from %p to %p\n",
                 UIP_ICMP_PAYLOAD,
                 (uint8_t *)UIP_ICMP_PAYLOAD + uip_len -
                 uip_l2_l3_icmp_hdr_len);
  while(locslhptr <
        (struct sequence_list_header *)((uint8_t *)UIP_ICMP_PAYLOAD +
                                        uip_len - uip_l2_l3_icmp_hdr_len)) {
    VERBOSE_PRINTF("ROLL TM: ICMPv6 In, seq hdr @ %p\n", locslhptr);

    if((locslhptr->flags & SEQUENCE_LIST_RES) != 0) {
      PRINTF("ROLL TM: ICMPv6 In, non-zero reserved bits\n");
      goto drop;
    }

    /* Drop unsupported Seed ID Lengths. S bit: 0->short, 1->long */
#if ROLL_TM_SHORT_SEEDS
    if(!SEQUENCE_LIST_GET_S(locslhptr)) {
      ROLL_TM_STATS_ADD(icmp_bad);
      goto drop;
    }
#else
    if(SEQUENCE_LIST_GET_S(locslhptr)) {
      ROLL_TM_STATS_ADD(icmp_bad);
      goto drop;
    }
#endif

    PRINTF("ROLL TM: ICMPv6 In, Sequence List for Seed ID ");
    PRINT_SEED(&locslhptr->seed_id);
    PRINTF(" M=%u, S=%u, Len=%u\n", SEQUENCE_LIST_GET_M(locslhptr),
           SEQUENCE_LIST_GET_S(locslhptr), locslhptr->seq_len);

    seq_ptr = (uint16_t *)((uint8_t *)locslhptr
                           + sizeof(struct sequence_list_header));
    end_ptr = (uint16_t *)((uint8_t *)locslhptr
                           + sizeof(struct sequence_list_header) +
                           locslhptr->seq_len * 2);

    /* Fetch a pointer to the corresponding trickle timer */
    loctpptr = &t[SEQUENCE_LIST_GET_M(locslhptr)];

    locswptr = NULL;

    /* Find the sliding window for this Seed ID */
    locswptr = window_lookup(&locslhptr->seed_id,
                             SEQUENCE_LIST_GET_M(locslhptr));

    /* If we have a window, iterate sequence values and check consistency */
    if(locswptr) {
      SLIDING_WINDOW_LISTED_SET(locswptr);
      locswptr->min_listed = -1;
      PRINTF("ROLL TM: ICMPv6 In, Window bounds [%u , %u]\n",
             locswptr->lower_bound, locswptr->upper_bound);
      for(; seq_ptr < end_ptr; seq_ptr++) {
        /* Check for "They have new" */
        /* If an advertised seq. val is GT our upper bound */
        val = uip_htons(*seq_ptr);
        PRINTF("ROLL TM: ICMPv6 In, Check seq %u @ %p\n", val, seq_ptr);
        if(SEQ_VAL_IS_GT(val, locswptr->upper_bound)) {
          PRINTF("ROLL TM: Inconsistency - Advertised Seq. ID %u GT upper"
                 " bound %u\n", val, locswptr->upper_bound);
          loctpptr->inconsistency = 1;
        }

        /* If an advertised seq. val is within our bounds */
        if((SEQ_VAL_IS_LT(val, locswptr->upper_bound) ||
            SEQ_VAL_IS_EQ(val, locswptr->upper_bound)) &&
           (SEQ_VAL_IS_GT(val, locswptr->lower_bound) ||
            SEQ_VAL_IS_EQ(val, locswptr->lower_bound))) {

          inconsistency = 1;
          /* Check if the advertised sequence is in our buffer */
          for(locmpptr = &buffered_msgs[ROLL_TM_BUFF_NUM - 1];
              locmpptr >= buffered_msgs; locmpptr--) {
            if(MCAST_PACKET_IS_USED(locmpptr) && locmpptr->sw == locswptr) {
              if(SEQ_VAL_IS_EQ(locmpptr->seq_val, val)) {

                inconsistency = 0;
                MCAST_PACKET_LISTED_SET(locmpptr);
                PRINTF("ROLL TM: ICMPv6 In, %u listed\n", locmpptr->seq_val);

                /* Update lowest seq. num listed for this window
                 * We need this to check for "we have new" */
                if(locswptr->min_listed == -1 ||
                   SEQ_VAL_IS_LT(val, locswptr->min_listed)) {
                  locswptr->min_listed = val;
                }
                break;
              }
            }
          }
          if(inconsistency) {
            PRINTF("ROLL TM: Inconsistency - ");
            PRINTF("Advertised Seq. ID %u within bounds", val);
            PRINTF(" [%u, %u] but no matching entry\n",
                   locswptr->lower_bound, locswptr->upper_bound);
            loctpptr->inconsistency = 1;
          }
        }
      }
    } else {
      /* A new sliding window in an ICMP message is not explicitly stated
       * in the draft as inconsistency. Until this is clarified, we consider
       * this to be a point where we diverge from the draft for performance
       * improvement reasons (or as some would say, 'this is an extension') */
      PRINTF("ROLL TM: Inconsistency - Advertised window unknown to us\n");
      loctpptr->inconsistency = 1;
    }
    locslhptr = (struct sequence_list_header *)(((uint8_t *)locslhptr) +
        sizeof(struct sequence_list_header) + (2 * locslhptr->seq_len));
  }
  /* Done parsing the message */

  /* Check for "We have new */
  PRINTF("ROLL TM: ICMPv6 In, Check our buffer\n");
  for(locmpptr = &buffered_msgs[ROLL_TM_BUFF_NUM - 1];
      locmpptr >= buffered_msgs; locmpptr--) {
    if(MCAST_PACKET_IS_USED(locmpptr)) {
      locswptr = locmpptr->sw;
      PRINTF("ROLL TM: ICMPv6 In, ");
      PRINTF("Check %u, Seed L: %u, This L: %u Min L: %d\n",
             locmpptr->seq_val, SLIDING_WINDOW_IS_LISTED(locswptr),
             MCAST_PACKET_IS_LISTED(locmpptr), locswptr->min_listed);

      /* Point to the sliding window's trickle param */
      loctpptr = &t[SLIDING_WINDOW_GET_M(locswptr)];
      if(!SLIDING_WINDOW_IS_LISTED(locswptr)) {
        /* If a buffered packet's Seed ID was not listed */
        PRINTF("ROLL TM: Inconsistency - Seed ID ");
        PRINT_SEED(&locswptr->seed_id);
        PRINTF(" was not listed\n");
        loctpptr->inconsistency = 1;
        MCAST_PACKET_SEND_SET(locmpptr);
      } else {
        /* This packet was not listed but a prior one was */
        if(!MCAST_PACKET_IS_LISTED(locmpptr) &&
           (locswptr->min_listed >= 0) &&
           SEQ_VAL_IS_GT(locmpptr->seq_val, locswptr->min_listed)) {
          PRINTF("ROLL TM: Inconsistency - ");
          PRINTF("Seq. %u was not listed but %u was\n",
                 locmpptr->seq_val, locswptr->min_listed);
          loctpptr->inconsistency = 1;
          MCAST_PACKET_SEND_SET(locmpptr);
        }
      }
    }
  }

drop:

  if(t[0].inconsistency) {
    reset_trickle_timer(0);
  } else {
    t[0].c++;
  }
  if(t[1].inconsistency) {
    reset_trickle_timer(1);
  } else {
    t[1].c++;
  }

  return;
}
/*---------------------------------------------------------------------------*/
static void
out()
{

  if(uip_len + HBHO_TOTAL_LEN > UIP_BUFSIZE) {
    PRINTF("ROLL TM: Multicast Out can not add HBHO. Packet too long\n");
    goto drop;
  }

  /* Slide 'right' by HBHO_TOTAL_LEN bytes */
  memmove(UIP_EXT_BUF_NEXT, UIP_EXT_BUF, uip_len - UIP_IPH_LEN);
  memset(UIP_EXT_BUF, 0, HBHO_TOTAL_LEN);

  UIP_EXT_BUF->next = UIP_IP_BUF->proto;
  UIP_EXT_BUF->len = 0;

  lochbhmptr = UIP_EXT_OPT_FIRST;
  lochbhmptr->type = HBHO_OPT_TYPE_TRICKLE;

  /* Set the sequence ID */
  last_seq = SEQ_VAL_ADD(last_seq, 1);
  lochbhmptr->flags = last_seq >> 8;
  lochbhmptr->seq_id_lsb = last_seq & 0xFF;
#if ROLL_TM_SHORT_SEEDS
  seed_id_cpy(&lochbhmptr->seed_id, &uip_lladdr.addr[UIP_LLADDR_LEN - 2]);
  lochbhmptr->len = HBHO_LEN_SHORT_SEED;
#else
  lochbhmptr->len = HBHO_LEN_LONG_SEED;
  /* PadN */
  lochbhmptr->padn_type = UIP_EXT_HDR_OPT_PADN;
  lochbhmptr->padn_len = 0;
#endif

  /* Set the M bit for our outgoing messages, if necessary */
#if ROLL_TM_SET_M_BIT
  HBH_SET_M(lochbhmptr);
#endif

  uip_ext_len += HBHO_TOTAL_LEN;
  uip_len += HBHO_TOTAL_LEN;

  /* Update the proto and length field in the v6 header */
  UIP_IP_BUF->proto = UIP_PROTO_HBHO;
  UIP_IP_BUF->len[0] = ((uip_len - UIP_IPH_LEN) >> 8);
  UIP_IP_BUF->len[1] = ((uip_len - UIP_IPH_LEN) & 0xff);

  PRINTF("ROLL TM: Multicast Out, HBHO: T=%u, L=%u, M=%u, S=0x%02x%02x\n",
         lochbhmptr->type, lochbhmptr->len, HBH_GET_M(lochbhmptr),
         HBH_GET_SV_MSB(lochbhmptr), lochbhmptr->seq_id_lsb);

  /*
   * We need to remember this message and advertise it in subsequent ICMP
   * messages. Otherwise, our neighs will think we are inconsistent and will
   * bounce it back to us.
   *
   * Queue this message but don't set its MUST_SEND flag. We reset the trickle
   * timer and we send it immediately. We then set uip_len = 0 to stop the core
   * from re-sending it.
   */
  if(accept(ROLL_TM_DGRAM_OUT)) {
    tcpip_output(NULL);
    UIP_MCAST6_STATS_ADD(mcast_out);
  }

drop:
  uip_slen = 0;
  uip_len = 0;
  uip_ext_len = 0;
}
/*---------------------------------------------------------------------------*/
static uint8_t
in()
{
  /*
   * We call accept() which will sort out caching and forwarding. Depending
   * on accept()'s return value, we then need to signal the core
   * whether to deliver this to higher layers
   */
  if(accept(ROLL_TM_DGRAM_IN) == UIP_MCAST6_DROP) {
    return UIP_MCAST6_DROP;
  }

  if(!uip_ds6_is_my_maddr(&UIP_IP_BUF->destipaddr)) {
    PRINTF("ROLL TM: Not a group member. No further processing\n");
    return UIP_MCAST6_DROP;
  } else {
    PRINTF("ROLL TM: Ours. Deliver to upper layers\n");
    UIP_MCAST6_STATS_ADD(mcast_in_ours);
    return UIP_MCAST6_ACCEPT;
  }
}
/*---------------------------------------------------------------------------*/
static void
init()
{
  PRINTF("ROLL TM: ROLL Multicast - Draft #%u\n", ROLL_TM_VER);

  memset(windows, 0, sizeof(windows));
  memset(buffered_msgs, 0, sizeof(buffered_msgs));
  memset(t, 0, sizeof(t));

  ROLL_TM_STATS_INIT();
  UIP_MCAST6_STATS_INIT(&stats);

  /* Register the ICMPv6 input handler */
  uip_icmp6_register_input_handler(&roll_tm_icmp_handler);

  for(iterswptr = &windows[ROLL_TM_WINS - 1]; iterswptr >= windows;
      iterswptr--) {
    iterswptr->lower_bound = -1;
    iterswptr->upper_bound = -1;
    iterswptr->min_listed = -1;
  }

  TIMER_CONFIGURE(0);
  reset_trickle_timer(0);
  TIMER_CONFIGURE(1);
  reset_trickle_timer(1);
  return;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief The ROLL TM engine driver
 */
const struct uip_mcast6_driver roll_tm_driver = {
  "ROLL TM",
  init,
  out,
  in,
};
/*---------------------------------------------------------------------------*/
/** @} */
