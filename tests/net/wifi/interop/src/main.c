/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The interop test is driven entirely from the Wi-Fi shell by the pytest
 * harness, so the application itself does nothing. The shell, networking and
 * the native_sim Wi-Fi driver are all started by their own init hooks.
 */

int main(void)
{
	return 0;
}
