//
// Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
//
// SPDX-License-Identifier: Apache-2.0
//

#include <zpp/timer.hpp>
#include <zpp/thread.hpp>
#include <zpp/fmt.hpp>
#include <zpp/sem.hpp>

namespace {

void timer_callback(zpp::timer_base* t) noexcept
{
	zpp::print("Hello from timer 3 tid={}\n", zpp::this_thread::get_id());
}

} // namespace

int main(int argc, char* argv[])
{
	using namespace zpp;
	using namespace std::chrono;

	auto t1 = timer(
		[] (auto t) {
			print("Hello from timer 1 tid={}\n",
					this_thread::get_id());
		} );

	t1.start(1s, 1s);


	auto t2 = timer(
		[] (auto t) {
			print("\nHello from single shot timer 2 tid={}\n\n",
					this_thread::get_id());
		} );

	t2.start(5s);

	auto t3 = timer(timer_callback);
	t3.start(6s, 1s);

	sem counter;

	auto lambda = [&counter] (zpp::timer_base* t)
	{
		if (counter.count() > 10) {
			counter.reset();
		}

		print("Hello from timer 4 tid={} a={}\n",
			this_thread::get_id(), counter.count());

	};

	auto t4 = timer(lambda);
	t4.start(1s, 1s);

	while(true) {
		//
		// print the thread ID of the main thread
		//
		print("Hello from main tid={}\n", this_thread::get_id());

		//
		// let main thread sleep for 1 s
		//
		this_thread::sleep_for(1s);

		counter++;
	}

	return 0;
}
