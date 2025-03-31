#ifndef DRIVERS_DALI_INCLUDE_DALI_STD_H_
#define DRIVERS_DALI_INCLUDE_DALI_STD_H_

/* definitions given by the DALI standards */

/* IEC 62396-101:2022 4.10 Table 4 Power interruptions */
#define FAILURE_CONDITION_US (550000U) /* 550 ms */

/* IEC 62386-101:2022 7.4 Frame types */
#define MAX_BIT_PER_FRAME     (32U)
#define FRAME_BACKWARD_LENGTH (8U)
#define FRAME_GEAR_LENGTH     (16U)
#define FRAME_DEVICE_LENGTH   (24U)
#define FRAME_UPDATE_LENGTH   (32U)

/* IEC 62386-101:2022 8.2.1 Receiver bit timing - Table 18 */
#define RX_BIT_TIME_HALF_MIN_US (333U)
#define RX_BIT_TIME_HALF_MAX_US (500U)
#define RX_BIT_TIME_FULL_MIN_US (667U)
#define RX_BIT_TIME_FULL_MAX_US (1000U)
#define RX_BIT_TIME_STOP_US     (2400U)

/* IEC 62386-101:2022 8.2.3 Receiver settling times - Table 20 */
#define RX_FORWARD_BACK_MAX_US (12400U)
#define RX_TWICE_MIN_US        (2400U)
#define RX_TWICE_MAX_US        (94000U)

/* IEC 62386-101:2022 8.3.1 Multi-master transmitter bit timing */
#define TX_HALF_BIT_MIN_US (400U)
#define TX_HALF_BIT_US     (417U)
#define TX_HALF_BIT_MAX_US (433U)
#define TX_FULL_BIT_MIN_US (800U)
#define TX_FULL_BIT_US     (833U)
#define TX_FULL_BIT_MAX_US (867U)
#define TX_STOP_BIT_US     (2450U)

/* IEC 62386-101:2022 8.3.2 Multi-master transmitter settling time values */
#define TX_BACKWARD_INTERFRAME_MIN_US (5500U)
#define TX_BACKWARD_INTERFRAME_MAX_US (10500U)
#define TX_PRIO_1_INTERFRAME_MIN_US   (13500U)
#define TX_PRIO_1_INTERFRAME_MAX_US   (14700U)
#define TX_PRIO_2_INTERFRAME_MIN_US   (14900U)
#define TX_PRIO_2_INTERFRAME_MAX_US   (16100U)
#define TX_PRIO_3_INTERFRAME_MIN_US   (16300U)
#define TX_PRIO_3_INTERFRAME_MAX_US   (17700U)
#define TX_PRIO_4_INTERFRAME_MIN_US   (17900U)
#define TX_PRIO_4_INTERFRAME_MAX_US   (19300U)
#define TX_PRIO_5_INTERFRAME_MIN_US   (19500U)
#define TX_PRIO_5_INTERFRAME_MAX_US   (21100U)

/* IEC 62386-101:2022 9.2.3 Collision detroy areas */
#define TX_DESTROY_1_MIN_US (100U)
#define TX_DESTROY_1_MAX_US (357U)
#define TX_DESTROY_2_MIN_US (477U)
#define TX_DESTROY_2_MAX_US (723U)
#define TX_DESTROY_3_MIN_US (943U)

/* IEC 62386-101:2022 9.2.4 Collision recovery timing */
#define TX_BREAK_MIN_US   (1200U)
#define TX_BREAK_MAX_US   (1400U)
#define TX_RECOVER_MIN_US (4000U)
#define TX_RECOVER_MAX_US (4600U)

/* IEC 62386-101:2022 9.6.2 Shared interface - backward frame */
#define TX_CORRUPT_BIT_MIN_US (1300U)
#define TX_CORRUPT_BIT_MAX_US (2000U)

#endif /* DRIVERS_DALI_INCLUDE_DALI_STD_H_ */