#define DT_DRV_COMPAT intel_hpet

#define MIN_DELAY 4

#define HPET_REG32(off) (*(volatile uint32_t *)(long) \
			 (CONFIG_HPET_TIMER_BASE_ADDRESS + (off)))

#define HPET_REG64(off) (*(volatile uint64_t *)	\
			 (CONFIG_HPET_TIMER_BASE_ADDRESS + (off)))
#if CONFIG_CLOCK_SOURCE_DOWNSCALED
#define CLK_PERIOD_REG        HPET_REG32(0x04) * CONFIG_CLK_SCALE       /* High dword of caps reg */
#else
#define CLK_PERIOD_REG        HPET_REG32(0x04)                          /* High dword of caps reg */
#endif
#define GENERAL_CONF_REG      HPET_REG32(0x10)
#define GENERAL_INT_STATUS_REG HPET_REG32(0x20)
#define MAIN_COUNTER_REG      HPET_REG64(0xf0)
#define MAIN_COUNTER_LSW_REG  HPET_REG32(0xf0)
#define MAIN_COUNTER_MSW_REG  HPET_REG32(0xf4)
#define TIMER0_CONF_REG       HPET_REG32(0x100)
#define TIMER0_COMPARATOR_REG HPET_REG64(0x108)
#define TIMER1_CONF_REG       HPET_REG32(0x120)
#define TIMER2_CONF_REG       HPET_REG32(0x140)
#define CONTROL_AND_STATUS_REG    HPET_REG32(0x160)

#define HPET_Tn_INT_ROUTE_CAP(caps) (caps > 32)
#define HPET_Tn_FSB_INT_DEL_CAP(caps) (caps & BIT(15))
#define HPET_Tn_FSB_EN_CNF BIT(14)
#define HPET_Tn_INT_ROUTE_CNF_MASK (0x1f << 9)
#define HPET_Tn_INT_ROUTE_CNF_SHIFT 9
#define HPET_Tn_32MODE_CNF BIT(8)
#define HPET_Tn_VAL_SET_CNF ((uint64_t)BIT(6))
#define HPET_Tn_SIZE_CAP(caps) (caps & BIT(5))
#define HPET_Tn_PER_INT_CAP(caps) (caps & BIT(4))
#define HPET_Tn_TYPE_CNF BIT(3)
#define HPET_Tn_INT_ENB_CNF ((uint64_t)BIT(2))
#define HPET_Tn_INT_TYPE_CNF BIT(1)
#define TIMER2_INT_ROUTE 0x0b

#define HPET_ENABLE_CNF BIT(0)
#define HPET_LEGACY_RT_CNF BIT(1)

/* TIMERn_CONF_REG bits */
#define TCONF_INT_ENABLE BIT(2)
#define TCONF_PERIODIC   BIT(3)
#define TCONF_VAL_SET    BIT(6)
#define TCONF_MODE32     BIT(8)

/* control and status register macros */
#define GENERAL_CONFIG          BIT(0)
#define GENERAL_INT_STATUS      BIT(1)
#define MAIN_COUNTER_VALUE      (3 << 2)
#define TIMER0_CONFIG_CAPS      BIT(4)
#define TIMER1_CONFIG_CAPS      BIT(5)
#define TIMER2_CONFIG_CAPS      BIT(6)
#define TIMER0_COMPARATOR       (3 << 7)
#define TIMER1_COMPARATOR       BIT(9)
#define TIMER2_COMPARATOR       BIT(10)
#define MAIN_COUNTER_INVALID    BIT(13)

#if CONFIG_CLOCK_SOURCE_DOWNSCALED
#define HPET_FREQ       32768 / CONFIG_CLK_SCALE
#else
#define HPET_FREQ       32768
#endif
#define HPET_ERR_FIX_PER_SEC    4
#define HPET_OS_TICKS_PER_ERR_FIX \
	(CONFIG_SYS_CLOCK_TICKS_PER_SEC / HPET_ERR_FIX_PER_SEC)
#define HPET_COUNTERS_PER_ERR_FIX (HPET_FREQ / HPET_ERR_FIX_PER_SEC)
#define HPET_COUNTERS_PER_OS_TICK (HPET_FREQ / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define PISECONDS_PER_SECOND (1000000000000ull)
#define TICK_MAX (0x7FFFFFFF)
