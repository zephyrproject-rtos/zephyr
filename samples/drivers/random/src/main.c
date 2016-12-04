/*
 * Copyright (c) 2016 ARM Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <random.h>
#include <stdio.h>

void main(void)
{
	struct device *dev;

	printf("Random Example! %s\n", CONFIG_ARCH);

	dev = device_get_binding(CONFIG_RANDOM_NAME);
	if (!dev) {
		printf("error: no random device\n");
		return;
	}

	printf("random device is %p, name is %s\n",
	       dev, dev->config->name);

	while (1) {
#define BUFFER_LENGTH 10
		uint8_t buffer[BUFFER_LENGTH];
		int r;

		r = random_get_entropy(dev, buffer, BUFFER_LENGTH);
		if (r) {
			printf("random_get_entropy failed: %d\n", r);
			break;
		};

		for (int i = 0; i < BUFFER_LENGTH; i++) {
			printf("  0x%x", buffer[i]);
		};

		printf("\n");

		k_sleep(1000);
	}
}
