/*
 * Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR__INCLUDE__ZPP__FMT_HPP
#define ZEPHYR__INCLUDE__ZPP__FMT_HPP

#include <zpp/compiler.hpp>

#include <sys/__assert.h>

#include <cstddef>
#include <chrono>

namespace zpp {

namespace internal {

inline void print_arg() noexcept { }
inline void print_arg(bool v) noexcept { printk("%d", (int)v); }
inline void print_arg(float v) noexcept { printk("%f", v); }
inline void print_arg(double v) noexcept { printk("%g", v); }
inline void print_arg(char v) noexcept { printk("%c", v); }
inline void print_arg(const char* v) noexcept { printk("%s", v); }
inline void print_arg(const void* v) noexcept { printk("%p", v); }
inline void print_arg(u8_t v) noexcept { printk("%d", (u32_t)v); }
inline void print_arg(s8_t v) noexcept { printk("%d", (s32_t)v); }
inline void print_arg(u16_t v) noexcept { printk("%d", (u32_t)v); }
inline void print_arg(s16_t v) noexcept { printk("%d", (s32_t)v); }
inline void print_arg(u32_t v) noexcept { printk("%d", v); }
inline void print_arg(s32_t v) noexcept { printk("%d", v); }
inline void print_arg(u64_t v) noexcept { printk("%lld", v); }
inline void print_arg(s64_t v) noexcept { printk("%lld", v); }

template < class Rep, class Period>
inline void print_arg(std::chrono::duration<Rep, Period> v)
{
	using namespace std::chrono;

	auto s = duration_cast<seconds>(v);
	v -= duration_cast<decltype(v)>(s);
	auto ms = duration_cast<milliseconds>(v);
	v -= duration_cast<decltype(v)>(ms);
	auto us = duration_cast<microseconds>(v);
	v -= duration_cast<decltype(v)>(us);
	auto ns = duration_cast<nanoseconds>(v);

	printk("%d.%03d%03d%03ds",
		(int)s.count(), (int)ms.count(),
		(int)us.count(), (int)ns.count());
}

template<class Clock>
inline void print_arg(std::chrono::time_point<Clock> v)
{
	print_arg(v.time_since_epoch());
}

inline void print_helper(const char* fmt) noexcept
{
	printk("%s", fmt);
}

template<class FirstArg, class ...Args>
inline void print_helper(const char* fmt,
			FirstArg first, Args... args) noexcept
{
	enum class state { normal, format, open_brace, close_brace, done };

	state s = state::normal;
	size_t n = 0;

	while (true) {
		char c = fmt[n++];

		if (c == '\0') {
			return;
		}

		switch (s) {
		case state::normal:
			switch (c) {
			case '{':
				s = state::open_brace;
				break;
			case '}':
				s = state::close_brace;
				break;
			default:
				printk("%c", c);
				break;
			}
			break;

		case state::open_brace:
			switch(c) {
			case '{':
				s = state::normal;
				printk("{");
				break;

			case '}':
				s = state::done;
				break;

			default:
				s = state::format;
				break;
			}
			break;

		case state::close_brace:
			if (c == '}') {
				printk("}");
			}
			s = state::normal;
			break;

		case state::format:
			switch (c) {
			case '}':
				s = state::done;
				break;
			default:
				break;
			}
			break;

		case state::done:
			break;

		}

		if (s == state::done) {
			break;
		}
	}

	print_arg(first);
	print_helper(&(fmt[n]), args...);
}

} // namespace internal

/**
 * @brief simple typesafe print function
 *
 * print uses the same format string syntax as the fmt C++ lib, just
 * that the feature set is very limited. It only supports {} without
 * anny options, for example print("Nr: {}", 1);
 *
 * @param fmt The format string using {} as place holder
 * @param args The needed arguments to print
 */
template<class ...Args>
inline void print(const char* fmt, Args... args) noexcept
{
	internal::print_helper(fmt, args...);
}

} // namespace zpp

#endif // ZEPHYR__INCLUDE__ZPP__FMT_HPP
