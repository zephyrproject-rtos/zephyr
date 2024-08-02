/*
 * Copyright 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <thrift/concurrency/Mutex.h>

namespace apache
{
namespace thrift
{
namespace concurrency
{

class Mutex::impl
{
public:
	k_spinlock_key_t key;
	struct k_spinlock lock;
};

Mutex::Mutex()
{
	impl_ = std::make_shared<Mutex::impl>();
}

void Mutex::lock() const
{
	while (!trylock()) {
		k_msleep(1);
	}
}

bool Mutex::trylock() const
{
	return k_spin_trylock(&impl_->lock, &impl_->key) == 0;
}

bool Mutex::timedlock(int64_t milliseconds) const
{
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(milliseconds));

	do {
		if (trylock()) {
			return true;
		}
		k_msleep(5);
	} while(!sys_timepoint_expired(end));

	return false;
}

void Mutex::unlock() const
{
	k_spin_unlock(&impl_->lock, impl_->key);
}

void *Mutex::getUnderlyingImpl() const
{
	return &impl_->lock;
}
} // namespace concurrency
} // namespace thrift
} // namespace apache
