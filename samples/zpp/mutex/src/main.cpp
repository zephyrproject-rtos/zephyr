//
// Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
//
// SPDX-License-Identifier: Apache-2.0
//

#include <zpp/thread.hpp>
#include <zpp/mutex.hpp>
#include <zpp/fmt.hpp>

ZPP_THREAD_TCB_DEFINE(tcb, 1024);

namespace {

zpp::mutex lock;

void thread_entry(int* a, int* b)
{
	using namespace std::chrono;

	while (true) {
		zpp::lock_guard g(lock);

		(*a)++;

		zpp::this_thread::sleep_for(250ms);

		(*b)++;

		zpp::this_thread::sleep_for(250ms);
	}
}

} // namespace

int main(int argc, char* argv[])
{
	int a{ 0 };
	int b{ 0 };

	zpp::this_thread::set_priority( zpp::thread_prio::lowest_preempt() );

	const zpp::thread_attr attr(
			zpp::this_thread::get_priority() + 1,
			zpp::thread_inherit_perms::yes,
			zpp::thread_suspend::no
		);

	auto t = zpp::thread(tcb, attr, thread_entry, &a, &b);

	while(true) {
		lock.lock();

		zpp::print("main {} {}\n", a, b);

		lock.unlock();
	}

	return 0;
}
