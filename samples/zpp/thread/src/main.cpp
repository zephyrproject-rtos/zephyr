//
// Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
//
// SPDX-License-Identifier: Apache-2.0
//

//
// Simple threading example the starts 2 threads. Both
// threads and the main thread will print out a msg in
// a endless loop.
//

#include <zpp/thread.hpp>
#include <zpp/fmt.hpp>

#include <chrono>

//
// using a anonymous namespace for file local variables and functions
//
namespace {

//
// Define the Thread Control Block for the first thread
// Use a stack of 1024 bytes
//
ZPP_THREAD_TCB_DEFINE(tcb1, 1024);

//
// Define the Thread Control Block for the second thread
// Use a stack of 1024 bytes
//
ZPP_THREAD_TCB_DEFINE(tcb2, 1024);

//
// Entry point of the second thread
// Taking 5 integers as arguments
//
void thread_2_entry(int a1, int a2, int a3, int a4, int a5)
{
	//
	// use zpp and std::chrono namespaces in this function
	//
	using namespace zpp;
	using namespace std::chrono;

	while (true) {
		//
		// print the thread ID and the 5 arguments
		//
		print("Hello from thread 2 "
			"tid={} a1={} a2={} a3={} a4={} a5={}\n",
			this_thread::get_id(), a1, a2, a3, a4, a5);


		//
		// let this thread sleep for 400 ms
		//
		this_thread::sleep_for(400ms);
	}
}

} // namespace


int main(int argc, char *argv[])
{
	//
	// use zpp and std::chrono namespaces in the function
	//
	using namespace zpp;
	using namespace std::chrono;

	//
	// Create thread attributes used for thread creation
	//
	const thread_attr attr(
				thread_prio::preempt(0),
				thread_inherit_perms::yes,
				thread_suspend::no
			);

	//
	// Create the first thread, using a lambda taking
	// no arguments
	//
	auto t1 = thread(
		tcb1, attr,
		[]() {
			while (true) {
				//
				// print the thread ID of this thread
				//
				print("Hello from thread 1 tid={}\n",
							this_thread::get_id());

				//
				// let this thread sleep for 500 ms
				//
				this_thread::sleep_for(500ms);
			}
		});

	//
	// Create the second thread using the thread_2_entry function
	// and passing 5 integers as arguments
	//
	auto t2 = thread(tcb2, attr, thread_2_entry, 1, 2, 3, 4, 5);

	//
	// Loop for every, becuase ending main would not only end the
	// program, t1 and t2 would also go out of scope aborting the
	// two threads.
	//
	while (true) {
		//
		// print the thread ID of the main thread
		//
		print("Hello from main tid={}\n", this_thread::get_id());

		//
		// let main thread sleep for 1 s
		//
		this_thread::sleep_for(1s);
	}

	return 0;
}
