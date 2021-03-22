/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_uart_basic
 * @{
 * @defgroup t_uart_fifo test_uart_pending
 * @brief TestPurpose: test UART uart_irq_is_pending()
 * @details
 *
 * Test if uart_irq_is_pending() correctly returns 0 when there are no
 * more RX and TX pending interrupts.
 *
 * The test consists in disabling TX IRQ so no TX interrupts are
 * generated and the TX IRQ pending flag is never set. At the same time
 * RX IRQ is enabled to let received data cause a RX IRQ and so set the
 * RX IRQ pending flag.
 *
 * Then a message is sent via serial to inform that the test is ready to
 * receive serial data, which will trigger a RX IRQ.
 *
 * Once a RX IRQ happens RX data is read by uart_fifo_read() until there
 * is no more RX data to be popped from FIFO and all IRQs are handled.
 * When that happens uart_irq_is_pending() is called and must return 0,
 * indicating there are no more pending interrupts to be processed. If 0
 * is returned the test passes.
 *
 * In some cases uart_irq_is_pending() does not correctly use the IRQ
 * pending flags to determine if there are pending interrupts, hence
 * even tho there aren't any further RX and TX IRQs to be processed it
 * wrongly returns 1. If 1 is returned the test fails.
 *
 * @}
 */

#include "test_uart.h"

#define MAX_NUM_TRIES 512
#define NOT_READY 0

#define FAILED	0
#define PASSED	1
#define WAIT	2
static int volatile status;

static void uart_pending_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	int num_tries = 0;
	char recv_char;

	/*
	 * If the bug is not present uart_fifo_read() will pop all
	 * received data until there is no more RX data, thus
	 * uart_irq_is_pending() must correctly return 0 indicating
	 * that there are no more RX interrupts to be processed.
	 * Otherwise uart_irq_is_pending() never returns 0 even tho
	 * there is no more RX data in the RX buffer to be processed,
	 * so, in that case, the test fails after MAX_NUM_TRIES attempts.
	 */
	status = PASSED;
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev) == NOT_READY) {
			if (num_tries < MAX_NUM_TRIES) {
				num_tries++;
				continue;
			} else {
				/*
				 * Bug: no more tries; uart_irq_is_pending()
				 * always returned 1 in spite of having no more
				 * RX data to be read from FIFO and no more TX
				 * data in FIFO to be sent via serial line.
				 * N.B. uart_irq_update() always returns 1, thus
				 * uart_irq_is_pending() got stuck without any
				 * real pending interrupt, i.e. no more RX and
				 * TX data to be popped or pushed from/to FIFO.
				 */
				status = FAILED;
				break;
			}
		}

		while (uart_fifo_read(dev, &recv_char, 1)) {
			/* Echo received char */
			TC_PRINT("%c", recv_char);
		}
	}
}

static int test_pending(void)
{
	const struct device *uart_dev = device_get_binding(UART_DEVICE_NAME);

	/*
	 * Set IRQ callback function to handle RX IRQ.
	 */
	uart_irq_callback_set(uart_dev, uart_pending_callback);

	/*
	 * Disable TX IRQ since transmitted data is not
	 * handled by uart_pending_callback() and we don't
	 * want to trigger any TX IRQ for this test.
	 */
	uart_irq_tx_disable(uart_dev);

	/*
	 * Enable RX IRQ so uart_pending_callback() can
	 * handle input data is available in RX FIFO.
	 */
	uart_irq_rx_enable(uart_dev);

	status = WAIT;

	/* Inform test is ready to receive data */
	TC_PRINT("Please send characters to serial console\n");

	while (status == WAIT) {
		/*
		 * Wait RX handler change 'status' properly:
		 * it will change to PASSED or FAILED after
		 * uart_irq_is_pending() is tested by
		 * uart_pending_callback() upon data reception.
		 */
	}

	if (status == PASSED) {
		return TC_PASS;
	} else {
		return TC_FAIL;
	}
}

void test_uart_pending(void)
{
	zassert_true(test_pending() == TC_PASS, NULL);
}
