//
// Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
//
// SPDX-License-Identifier: Apache-2.0
//

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <zpp/atomic_var.hpp>
#include <zpp/atomic_bitset.hpp>

#include <cassert>

zpp::atomic_bitset<320> g_bitset;

int main(int argc, char* argv[])
{
	using namespace zpp;

	g_bitset.store(1, true);
	assert(g_bitset.load(1) == true);

	g_bitset.store(330, true);
	assert(g_bitset.load(330) == true);

	// TODO

	return 0;
}
