/*
 * Copyright 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#else
#define printk printf
#endif

#include <iostream>

#include "Hello.h"

class HelloHandler : virtual public HelloIf
{
public:
	HelloHandler() : count(0)
	{
	}

	void ping()
	{
		printk("%s\n", __func__);
	}

	void echo(std::string &_return, const std::string &msg)
	{
		printk("%s: %s\n", __func__, msg.c_str());
		_return = msg;
	}

	int32_t counter()
	{
		++count;
		printk("%s: %d\n", __func__, count);
		return count;
	}

protected:
	int count;
};
