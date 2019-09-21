//
// Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
//
// SPDX-License-Identifier: Apache-2.0
//

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <zpp/clock.hpp>
#include <zpp/thread.hpp>
#include <zpp/fmt.hpp>

#include <chrono>

int main(int argc, char* argv[])
{
	using namespace zpp;
	using namespace std::chrono;

	while (true) {
		print("uptime_clock={} cyles_clock={}\n",
			uptime_clock::now(), cycle_clock::now());

		this_thread::sleep_for(100ms);
	}

	return 0;
}
