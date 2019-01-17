/* Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef NRF_802154_CONFIG_H__
#define NRF_802154_CONFIG_H__

#ifdef NRF_802154_PROJECT_CONFIG
#include NRF_802154_PROJECT_CONFIG
#endif

#include <nrf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_config 802.15.4 driver configuration
 * @{
 * @ingroup nrf_802154
 * @brief Configuration of the 802.15.4 radio driver for nRF SoCs.
 */

/**
 * @defgroup nrf_802154_config_radio Radio driver configuration
 * @{
 */

/**
 * @def NRF_802154_CCA_MODE_DEFAULT
 *
 * CCA mode used by the driver.
 *
 */
#ifndef NRF_802154_CCA_MODE_DEFAULT
#define NRF_802154_CCA_MODE_DEFAULT NRF_RADIO_CCA_MODE_ED
#endif

/**
 * @def NRF_802154_CCA_ED_THRESHOLD_DEFAULT
 *
 * Energy detection threshold used in CCA procedure.
 *
 */
#ifndef NRF_802154_CCA_ED_THRESHOLD_DEFAULT
#define NRF_802154_CCA_ED_THRESHOLD_DEFAULT 0x14
#endif

/**
 * @def NRF_802154_CCA_CORR_THRESHOLD_DEFAULT
 *
 * Correlator threshold used in CCA procedure.
 *
 */
#ifndef NRF_802154_CCA_CORR_THRESHOLD_DEFAULT
#define NRF_802154_CCA_CORR_THRESHOLD_DEFAULT 0x14
#endif

/**
 * @def NRF_802154_CCA_CORR_LIMIT_DEFAULT
 *
 * Correlator limit used in CCA procedure.
 *
 */
#ifndef NRF_802154_CCA_CORR_LIMIT_DEFAULT
#define NRF_802154_CCA_CORR_LIMIT_DEFAULT 0x02
#endif

/**
 * @def NRF_802154_INTERNAL_RADIO_IRQ_HANDLING
 *
 * If the driver should internally handle the RADIO IRQ.
 * If the driver is used in an OS, the RADIO IRQ may be handled by the OS and passed to
 * the driver by @ref nrf_802154_radio_irq_handler. In this case, internal handling should be disabled.
 */

#ifndef NRF_802154_INTERNAL_RADIO_IRQ_HANDLING

#if RAAL_SOFTDEVICE || RAAL_REM
#define NRF_802154_INTERNAL_RADIO_IRQ_HANDLING 0
#else // RAAL_SOFTDEVICE || RAAL_REM
#define NRF_802154_INTERNAL_RADIO_IRQ_HANDLING 1
#endif  // RAAL_SOFTDEVICE || RAAL_REM

#endif // NRF_802154_INTERNAL_RADIO_IRQ_HANDLING

/**
 * @def NRF_802154_IRQ_PRIORITY
 *
 * Interrupt priority for RADIO peripheral.
 * Keep IRQ priority high (low number) to prevent losing frames due to
 * preemption.
 *
 */
#ifndef NRF_802154_IRQ_PRIORITY
#define NRF_802154_IRQ_PRIORITY 0
#endif

/**
 * @def NRF_802154_TIMER_INSTANCE
 *
 * Timer instance used by the driver for ACK IFS and by the FEM module.
 *
 */
#ifndef NRF_802154_TIMER_INSTANCE
#define NRF_802154_TIMER_INSTANCE NRF_TIMER1
#endif

/**
 * @def NRF_802154_COUNTER_TIMER_INSTANCE
 *
 * Timer instance used by the driver for detecting when PSDU is being received.
 *
 * @note This configuration is used only when NRF_RADIO_EVENT_BCMATCH event handling is disabled
 *       (see @ref NRF_802154_DISABLE_BCC_MATCHING).
 */
#ifndef NRF_802154_COUNTER_TIMER_INSTANCE
#define NRF_802154_COUNTER_TIMER_INSTANCE NRF_TIMER0
#endif

/**
 * @def NRF_802154_SWI_EGU_INSTANCE
 *
 * SWI EGU instance used by the driver to synchronize PPIs and for requests and notifications if
 * SWI is in use.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *
 */
#ifndef NRF_802154_SWI_EGU_INSTANCE
#define NRF_802154_SWI_EGU_INSTANCE NRF_EGU3
#endif

/**
 * @def NRF_802154_SWI_IRQ_HANDLER
 *
 * SWI EGU IRQ handler used by the driver for requests and notifications if SWI is in use.
 *
 * @note This option is used when the driver uses SWI to process requests and notifications.
 *
 */
#ifndef NRF_802154_SWI_IRQ_HANDLER
#define NRF_802154_SWI_IRQ_HANDLER SWI3_EGU3_IRQHandler
#endif

/**
 * @def NRF_802154_SWI_IRQN
 *
 * SWI EGU IRQ number used by the driver for requests and notifications if SWI is in use.
 *
 * @note This option is used when the driver uses SWI to process requests and notifications.
 *
 */
#ifndef NRF_802154_SWI_IRQN
#define NRF_802154_SWI_IRQN SWI3_EGU3_IRQn
#endif

/**
 * @def NRF_802154_SWI_PRIORITY
 *
 * Priority of software interrupt used for requests and notifications.
 *
 */
#ifndef NRF_802154_SWI_PRIORITY
#define NRF_802154_SWI_PRIORITY 5
#endif

/**
 * @def NRF_802154_USE_RAW_API
 *
 * When this flag is set, RAW API is available for the MAC layer. It is recommended to use RAW API
 * because it provides more optimized functions.
 *
 * @note If RAW API is not available for the MAC layer, only less optimized functions performing
 *       copy are available.
 *
 */
#ifndef NRF_802154_USE_RAW_API
#define NRF_802154_USE_RAW_API 1
#endif

/**
 * @def NRF_802154_PENDING_SHORT_ADDRESSES
 *
 * Number of slots containing short addresses of nodes for which pending data is stored.
 *
 */
#ifndef NRF_802154_PENDING_SHORT_ADDRESSES
#define NRF_802154_PENDING_SHORT_ADDRESSES 10
#endif

/**
 * @def NRF_802154_PENDING_EXTENDED_ADDRESSES
 *
 * Number of slots containing extended addresses of nodes for which pending data is stored.
 *
 */
#ifndef NRF_802154_PENDING_EXTENDED_ADDRESSES
#define NRF_802154_PENDING_EXTENDED_ADDRESSES 10
#endif

/**
 * @def NRF_802154_RX_BUFFERS
 *
 * Number of buffers in receive queue.
 *
 */
#ifndef NRF_802154_RX_BUFFERS
#define NRF_802154_RX_BUFFERS 16
#endif

/**
 * @def NRF_802154_DISABLE_BCC_MATCHING
 *
 * Setting this flag disables NRF_RADIO_EVENT_BCMATCH handling, and therefore address filtering
 * during frame reception. With this flag set to 1, address filtering is done after receiving
 * a frame, during NRF_RADIO_EVENT_END handling.
 *
 */
#ifndef NRF_802154_DISABLE_BCC_MATCHING
#define NRF_802154_DISABLE_BCC_MATCHING 0
#endif

/**
 * @def NRF_802154_NOTIFY_CRCERROR
 *
 * With this flag set to 1, CRC errors are notified to upper layers. This requires an interrupt
 * handler to be used.
 *
 */
#ifndef NRF_802154_NOTIFY_CRCERROR
#define NRF_802154_NOTIFY_CRCERROR 1
#endif

/**
 * @def NRF_802154_FRAME_TIMESTAMP_ENABLED
 *
 * If timestamps should be added to received frames.
 * Enabling this feature enables the functions @ref nrf_802154_received_timsestamp_raw,
 * @ref nrf_802154_received_timestamp, @ref nrf_802154_transmitted_timestamp_raw, and
 * @ref nrf_802154_transmitted_timestamp, which add timestamps to received frames.
 *
 */
#ifndef NRF_802154_FRAME_TIMESTAMP_ENABLED
#define NRF_802154_FRAME_TIMESTAMP_ENABLED 1
#endif

/**
 * @def NRF_802154_DELAYED_TRX_ENABLED
 *
 * If delayed transmission and receive window features are available.
 *
 */
#ifndef NRF_802154_DELAYED_TRX_ENABLED
#define NRF_802154_DELAYED_TRX_ENABLED 1
#endif

/**
 * @}
 * @defgroup nrf_802154_config_clock Clock driver configuration
 * @{
 */

/**
 * @def NRF_802154_CLOCK_IRQ_PRIORITY
 *
 * Priority of clock interrupt used in standalone clock driver implementation.
 *
 * @note This configuration is only applicable for the Clock Abstraction Layer implementation
 *       in nrf_802154_clock_nodrv.c.
 *
 */
#ifndef NRF_802154_CLOCK_IRQ_PRIORITY
#define NRF_802154_CLOCK_IRQ_PRIORITY 10
#endif

/**
 * @def NRF_802154_CLOCK_LFCLK_SOURCE
 *
 * Low-frequency clock source used in standalone clock driver implementation.
 *
 * @note This configuration is only applicable for the Clock Abstraction Layer implementation
 *       in nrf_802154_clock_nodrv.c.
 *
 */
#ifndef NRF_802154_CLOCK_LFCLK_SOURCE
#define NRF_802154_CLOCK_LFCLK_SOURCE NRF_CLOCK_LFCLK_Xtal
#endif

/**
 * @}
 * @defgroup nrf_802154_config_rtc RTC driver configuration
 * @{
 */

/**
 * @def NRF_802154_RTC_IRQ_PRIORITY
 *
 * Priority of RTC interrupt used in standalone timer driver implementation.
 *
 * @note This configuration is only applicable for the Low Power Timer Abstraction Layer implementation
 *       in nrf_802154_lp_timer_nodrv.c.
 *
 */
#ifndef NRF_802154_RTC_IRQ_PRIORITY
#define NRF_802154_RTC_IRQ_PRIORITY 6
#endif

/**
 * @def NRF_802154_RTC_INSTANCE
 *
 * RTC instance used in standalone timer driver implementation.
 *
 * @note This configuration is only applicable for the Low Power Timer Abstraction Layer implementation
 *       in nrf_802154_lp_timer_nodrv.c.
 *
 */
#ifndef NRF_802154_RTC_INSTANCE
#define NRF_802154_RTC_INSTANCE NRF_RTC2
#endif

/**
 * @def NRF_802154_RTC_IRQ_HANDLER
 *
 * RTC interrupt handler name used in standalone timer driver implementation.
 *
 * @note This configuration is only applicable for Low Power Timer Abstraction Layer implementation
 *       in nrf_802154_lp_timer_nodrv.c.
 *
 */
#ifndef NRF_802154_RTC_IRQ_HANDLER
#define NRF_802154_RTC_IRQ_HANDLER RTC2_IRQHandler
#endif

/**
 * @def NRF_802154_RTC_IRQN
 *
 * RTC Interrupt number used in standalone timer driver implementation.
 *
 * @note This configuration is only applicable for the Low Power Timer Abstraction Layer implementation
 *       in nrf_802154_lp_timer_nodrv.c.
 *
 */
#ifndef NRF_802154_RTC_IRQN
#define NRF_802154_RTC_IRQN RTC2_IRQn
#endif

/**
 * @}
 * @defgroup nrf_802154_config_csma CSMA/CA procedure configuration
 * @{
 */

/**
 * @def NRF_802154_CSMA_CA_ENABLED
 *
 * If CSMA-CA should be enabled by the driver. Disabling CSMA-CA improves
 * driver performance.
 *
 */
#ifndef NRF_802154_CSMA_CA_ENABLED
#define NRF_802154_CSMA_CA_ENABLED 1
#endif

/**
 * @def NRF_802154_CSMA_CA_MIN_BE
 *
 * The minimum value of the backoff exponent (BE) in the CSMA-CA algorithm
 * (IEEE 802.15.4-2015: 6.2.5.1).
 *
 */
#ifndef NRF_802154_CSMA_CA_MIN_BE
#define NRF_802154_CSMA_CA_MIN_BE 3
#endif

/**
 * @def NRF_802154_CSMA_CA_MAX_BE
 *
 * The maximum value of the backoff exponent, BE, in the CSMA-CA algorithm
 * (IEEE 802.15.4-2015: 6.2.5.1).
 *
 */
#ifndef NRF_802154_CSMA_CA_MAX_BE
#define NRF_802154_CSMA_CA_MAX_BE 5
#endif

/**
 * @def NRF_802154_CSMA_CA_MAX_CSMA_BACKOFFS
 *
 * The maximum number of backoffs that the CSMA-CA algorithm will attempt before declaring a channel
 * access failure.
 *
 */
#ifndef NRF_802154_CSMA_CA_MAX_CSMA_BACKOFFS
#define NRF_802154_CSMA_CA_MAX_CSMA_BACKOFFS 4
#endif

/**
 * @}
 * @defgroup nrf_802154_config_timeout ACK timeout feature configuration
 * @{
 */

/**
 * @def NRF_802154_ACK_TIMEOUT_ENABLED
 *
 * If the ACK timeout feature should be enabled in the driver.
 *
 */
#ifndef NRF_802154_ACK_TIMEOUT_ENABLED
#define NRF_802154_ACK_TIMEOUT_ENABLED 1
#endif

/**
 * @def NRF_802154_ACK_TIMEOUT_DEFAULT_TIMEOUT
 *
 * Default timeout in us for the ACK timeout feature.
 *
 */
#ifndef NRF_802154_ACK_TIMEOUT_DEFAULT_TIMEOUT
#define NRF_802154_ACK_TIMEOUT_DEFAULT_TIMEOUT 7000
#endif

/**
 * @def NRF_802154_ACK_TIMEOUT_DEFAULT_TIMEOUT
 *
 * Default time-out in us for the precise ACK time-out feature.
 *
 */
#ifndef NRF_802154_PRECISE_ACK_TIMEOUT_DEFAULT_TIMEOUT
#define NRF_802154_PRECISE_ACK_TIMEOUT_DEFAULT_TIMEOUT 210
#endif

/**
 * @}
 * @defgroup nrf_802154_config_transmission Transmission start notification feature configuration
 * @{
 */

/**
 * @def NRF_802154_TX_STARTED_NOTIFY_ENABLED
 *
 * If notifications of started transmissions should be enabled in the driver.
 *
 * @note This feature is enabled by default if the ACK timeout feature or CSMA-CA is enabled.
 *       These features depend on notifications of transmission start.
 */
#ifndef NRF_802154_TX_STARTED_NOTIFY_ENABLED
#if NRF_802154_ACK_TIMEOUT_ENABLED || NRF_802154_CSMA_CA_ENABLED
#define NRF_802154_TX_STARTED_NOTIFY_ENABLED 1
#else
#define NRF_802154_TX_STARTED_NOTIFY_ENABLED 0
#endif
#endif // NRF_802154_TX_STARTED_NOTIFY_ENABLED

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif // NRF_802154_CONFIG_H__

/**
 *@}
 **/
