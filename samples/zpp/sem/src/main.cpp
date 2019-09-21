//
// Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
//
// SPDX-License-Identifier: Apache-2.0
//

#include <zpp/thread.hpp>
#include <zpp/sem.hpp>
#include <zpp/fmt.hpp>

ZPP_THREAD_TCB_DEFINE(tcb1, 1024);
ZPP_THREAD_TCB_DEFINE(tcb2, 1024);
ZPP_THREAD_TCB_DEFINE(tcb3, 1024);

namespace {

void thread_entry(zpp::sem* sem_ptr)
{
	using namespace zpp;
	using namespace std::chrono;

	while (true)
	{
		this_thread::sleep_for(100ms);

		if (sem_ptr->take()) {
			print("thread {} sem count {}\n",
						this_thread::get_id(),
						sem_ptr->count());
		} else {
			print("thread {} failed to take semaphore\n",
						this_thread::get_id());
		}
	}
}

} // namespace

int main(int argc, char* argv[])
{
	using namespace zpp;
	using namespace std::chrono;

	this_thread::set_priority(thread_prio::lowest_preempt());

	sem s;

	const thread_attr attr(
			this_thread::get_priority() + 1,
			thread_inherit_perms::yes,
			thread_suspend::no
		);

	auto t1 = thread(tcb1, attr, thread_entry, &s);
	auto t2 = thread(tcb2, attr, thread_entry, &s);
	auto t3 = thread(tcb3, attr, thread_entry, &s);

	while(true) {
		this_thread::sleep_for(100ms);
		s++;
	}

	return 0;
}
