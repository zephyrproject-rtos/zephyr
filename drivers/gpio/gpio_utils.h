/*
 * Copyright (c) 2016 Intel Corporation.
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

/**
 * @file Header where utility code can be found for GPIO drivers
 */

#ifndef __GPIO_UTILS_H__
#define __GPIO_UTILS_H__


/**
 * @brief Generic function to insert or remove a callback from a callback list
 *
 * @param callbacks A pointer to the original list of callbacks (can be NULL)
 * @param callback A pointer of the callback to insert or remove from the list
 * @param set A boolean indicating insertion or removal of the callback
 */
static inline void _gpio_manage_callback(sys_slist_t *callbacks,
					 struct gpio_callback *callback,
					 bool set)
{
	__ASSERT(callback, "No callback!");
	__ASSERT(callback->handler, "No callback handler!");

	if (set) {
		sys_slist_prepend(callbacks, &callback->node);
	} else {
		sys_slist_find_and_remove(callbacks, &callback->node);
	}
}

/**
 * @brief Generic function to go through and fire callback from a callback list
 *
 * @param list A pointer on the gpio callback list
 * @param port A pointer on the gpio driver instance
 * @param pins The actual pin mask that triggered the interrupt
 */
static inline void _gpio_fire_callbacks(sys_slist_t *list,
					struct device *port,
					uint32_t pins)
{
	struct gpio_callback *cb;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(list, node) {
		cb = (struct gpio_callback *)node;

		if (cb->pin_mask & pins) {
			__ASSERT(cb->handler, "No callback handler!");
			cb->handler(port, cb, pins);
		}
	}
}

#endif /* __GPIO_UTILS_H__ */
