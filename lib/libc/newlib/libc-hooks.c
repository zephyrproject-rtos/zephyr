/*
 * Copyright (c) 2015, Intel Corporation.
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

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <linker-defs.h>
#include <misc/util.h>

#define USED_RAM_END_ADDR   POINTER_TO_UINT(&_end)
#if defined(CONFIG_ARM)
#define USED_RAM_SIZE  (USED_RAM_END_ADDR - CONFIG_SRAM_BASE_ADDRESS)
#define MAX_HEAP_SIZE ((KB(CONFIG_SRAM_SIZE)) - USED_RAM_SIZE)
#elif defined(CONFIG_ARC)
#define USED_RAM_SIZE  (USED_RAM_END_ADDR - CONFIG_RAM_START)
#define MAX_HEAP_SIZE ((KB(CONFIG_RAM_SIZE)) - USED_RAM_SIZE)
#else  /* X86 */
#define USED_RAM_SIZE  (USED_RAM_END_ADDR - CONFIG_PHYS_RAM_ADDR)
#define MAX_HEAP_SIZE ((KB(CONFIG_RAM_SIZE)) - USED_RAM_SIZE)
#endif

static unsigned char *heap_base = UINT_TO_POINTER(USED_RAM_END_ADDR);
static unsigned int heap_sz;

static int _stdout_hook_default(int c)
{
	(void)(c);  /* Prevent warning about unused argument */

	return EOF;
}

static int (*_stdout_hook)(int) = _stdout_hook_default;

void __stdout_hook_install(int (*hook)(int))
{
	_stdout_hook = hook;
}

static unsigned char _stdin_hook_default(void)
{
	return 0;
}

static unsigned char (*_stdin_hook)(void) = _stdin_hook_default;

void __stdin_hook_install(unsigned char (*hook)(void))
{
	_stdin_hook = hook;
}

int read(int fd, char *buf, int nbytes)
{
	int i = 0;

	for (i = 0; i < nbytes; i++) {
		*(buf + i) = _stdin_hook();
		if ((*(buf + i) == '\n') || (*(buf + i) == '\r')) {
			i++;
			break;
		}
	}
	return i;
}

int write(int fd, char *buf, int nbytes)
{
	int i;

	for (i = 0; i < nbytes; i++) {
		if (*(buf + i) == '\n') {
			_stdout_hook('\r');
		}
		_stdout_hook(*(buf + i));
	}
	return nbytes;
}

int fstat(int fd, struct stat *buf)
{
	buf->st_mode = S_IFCHR;       /* Always pretend to be a tty */
	buf->st_blksize = 0;

	return 0;
}

int isatty(int file)
{
	return 1;
}


int kill(int i, int j)
{
	return 0;
}

int getpid(void)
{
	return 0;
}

int _fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}


void _exit(int status)
{
	write(1, "exit", 4);
	while (1) {
		;
	}
}

int close(int file)
{
	return -1;
}

int lseek(int file, int ptr, int dir)
{
	return 0;
}

void *sbrk(int count)
{
	void *ptr = heap_base + heap_sz;

	if ((heap_sz + count) < MAX_HEAP_SIZE) {
		heap_sz += count;
		return ptr;
	} else {
		return (void *)-1;
	}
}
