/*
 * Copyright (c) 2015 Intel Corporation
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

#include <ztest.h>

#define BUF_SZ 1024

static int pos;
char ram_console[BUF_SZ];

extern int (*_char_out)(int);
int (*_old_char_out)(int);

char *expected = "22 113 10000 32768 40000 22\n"
		 "p 112 -10000 -32768 -40000 -22\n"
		 "0xcafebabe 0x0000beef\n"
;


size_t stv = 22;
unsigned char uc = 'q';
unsigned short int usi = 10000;
unsigned int ui = 32768;
unsigned long ul = 40000;

/* FIXME
 * we know printk doesn't have full support for 64-bit values.
 * at least show it can print uint64_t values less than 32-bits wide
 */
unsigned long long ull = 22;

char c = 'p';
signed short int ssi = -10000;
signed int si = -32768;
signed long sl = -40000;
signed long long sll = -22;

uint32_t hex = 0xCAFEBABE;

void *ptr = (void *)0xBEEF;

static int ram_console_out(int character)
{
	ram_console[pos] = (char)character;
	pos = (pos + 1) % BUF_SZ;
	return _old_char_out(character);
}

void printk_test(void)
{
	_old_char_out = _char_out;
	_char_out = ram_console_out;

	printk("%zu %hhu %hu %u %lu %llu\n", stv, uc, usi, ui, ul, ull);
	printk("%c %hhd %hd %d %ld %lld\n", c, c, ssi, si, sl, sll);
	printk("0x%x %p\n", hex, ptr);

	ram_console[pos] = '\0';
	assert_true((strcmp(ram_console, expected) == 0), "printk failed");
}
