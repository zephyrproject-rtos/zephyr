/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

extern int hci_uart_main(void);

int main(void)
{
	hci_uart_main();
	return 0;
}
