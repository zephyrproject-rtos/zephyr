#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>

#include <inc/hw_rtc.h>
#include <inc/hw_memmap.h>
#include <inc/hw_systim.h>

#include <string.h>

#define RTC_ALARM_ENABLE 1

const struct device *const rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

struct k_sem update_semaphore;

#if defined(CONFIG_RTC_UPDATE)

int update_count;

static void update_callback(const struct device *dev, void *user_data)
{
	int *counter = (int *)user_data;

	(*counter)++;

	k_sem_give(&update_semaphore);

	printf("Update count: %d\r\n", *counter);
}

#endif /* CONFIG_RTC_UPDATE */

#define TIME(year, month, mday, hour, minute, second)                                              \
	{                                                                                          \
		.tm_year = year - 1900, .tm_mon = month - 1, .tm_mday = mday, .tm_hour = hour,     \
		.tm_min = minute, .tm_sec = second,                                                \
	}

#define PRINT_TIME(x)                                                                              \
	printf("RTC date and time: %04d-%02d-%02d %02d:%02d:%02d\r\n", x.tm_year + 1900,           \
	       x.tm_mon + 1, x.tm_mday, x.tm_hour, x.tm_min, x.tm_sec);

static void test_set_get_time(const struct device *rtc)
{
	int ret = 0;
	int expected_seconds = 0;

	struct rtc_time read_time;

	struct rtc_time test_time[] = {
		TIME(2024, 11, 17, 1, 15, 0),
		TIME(2024, 11, 17, 15, 30, 15),
	};

	for (int idx = 0; idx < ARRAY_SIZE(test_time); idx++) {
		ret = rtc_set_time(rtc, &test_time[idx]);

		if (ret < 0) {
			printf("Failed to set RTC time ! (reason: %d)\r\n", ret);
		}

		ret = rtc_get_time(rtc, &read_time);

		if (ret < 0) {
			printf("Unable to get time ! (reason: %d)\r\n", ret);
		}

		if ((read_time.tm_sec != test_time[idx].tm_sec) ||
		    (read_time.tm_min != test_time[idx].tm_min) ||
		    (read_time.tm_hour != test_time[idx].tm_hour) ||
		    (read_time.tm_mday != test_time[idx].tm_mday) ||
		    (read_time.tm_mon != test_time[idx].tm_mon) ||
		    (read_time.tm_year != test_time[idx].tm_year)) {
			printf("Set time and read time are not the same !\r\n");
		}

		expected_seconds = test_time[idx].tm_sec;

		for (int itr = 0; itr < 5; itr++) {
			expected_seconds++;
			expected_seconds %= 60;

			k_msleep(1000);

			ret = rtc_get_time(rtc, &read_time);

			if (ret != 0) {
				printf("Unable to get time from RTC ! (reason: %d)\r\n", ret);
			}

			if (read_time.tm_sec != expected_seconds) {
				printf("Read time (seconds) from RTC not equal to expected "
				       "value\r\n");
			}

			printf("Set RTC time:\r\n");
			PRINT_TIME(test_time[idx]);
			printf("Read RTC time (after %d seconds):\r\n", (itr + 1));
			PRINT_TIME(read_time);
		}
	}
}

#if CONFIG_RTC_UPDATE

static void test_rtc_update(const struct device *rtc)
{
	int ret;

	struct rtc_time current_time;

	printf("Starting RTC update callback test.\r\n");

	ret = rtc_update_set_callback(rtc, update_callback, &update_count);

	if (ret < 0) {
		printf("Unable to set RTC update callback.\r\n");
		return;
	}

	for (int i = 0; i < 5; i++) {

		printf("Waiting for update ...\r\n");

		k_sem_take(&update_semaphore, K_MSEC(1100));

		rtc_get_time(rtc, &current_time);
		PRINT_TIME(current_time);

		if (update_count == (i + 1)) {
			printf("Update count is incremented as expected.\r\n");
		} else {
			printf("Update count was not incremented to expected value! (update count "
			       "= %d)\r\n",
			       update_count);
		}
	}

	ret = rtc_update_set_callback(rtc, NULL, NULL);

	if (ret < 0) {
		printf("Unable to disable RTC update function.\r\n");
		return;
	}
}

#endif /* CONFIG_RTC_UPDATE */

int main(void)
{
	/* Check if the RTC is ready */
	if (!device_is_ready(rtc)) {
		printk("Device is not ready\n");
		return 0;
	}

	k_sem_init(&update_semaphore, 0, 1);

	while (1) {

		test_set_get_time(rtc);

#if defined(CONFIG_RTC_UPDATE)
		test_rtc_update(rtc);
#endif /* CONFIG_RTC_UPDATE  */

		k_sleep(K_FOREVER);
	};

	return 0;
}
