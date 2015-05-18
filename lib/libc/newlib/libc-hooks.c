#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#define HEAP_SIZE 4096
unsigned char heap[HEAP_SIZE];
unsigned int heap_sz = 0;

static int _stdout_hook_default(int c) {
        (void)(c);  /* Prevent warning about unused argument */

        return EOF;
}

static int (*_stdout_hook)(int) = _stdout_hook_default;

void __stdout_hook_install(int (*hook)(int)) {
        _stdout_hook = hook;
}

static unsigned char _stdin_hook_default(void) {
        return 0;
}

static unsigned char (*_stdin_hook)(void) = _stdin_hook_default;

void __stdin_hook_install(unsigned char (*hook)(void)) {
        _stdin_hook = hook;
}

int read( int fd, char *buf, int nbytes) {
	int i = 0;

	for (i = 0; i < nbytes; i++) {
		*(buf + i) = _stdin_hook();
		if ((*(buf + i) == '\n') || (*(buf + i) == '\r')) {
			i++;
			break;
		}
	}
	return (i);
}

int write(int fd, char *buf, int nbytes) {
	int i;

	for (i = 0; i < nbytes; i++) {
		if (*(buf + i) == '\n') {
			_stdout_hook ('\r');
		}
		_stdout_hook (*(buf + i));
	}
	return (nbytes);
}

int fstat( int fd, struct stat *buf) {
	buf->st_mode = S_IFCHR;       /* Always pretend to be a tty */
	buf->st_blksize = 0;

	return (0);
}

int isatty(int file) {
	return 1;
}


int kill(int i, int j) {
	return 0;
}

int getpid() {
	return 0;
}

int _fstat(int file, struct stat *st) {
	st->st_mode = S_IFCHR;
	return 0;
}


void exit(int status) {
	write(1, "exit", 4);
	while (1) {
		;
	}
}

int close(int file) {
	return -1;
}

int lseek(int file, int ptr, int dir) {
	return 0;
}

void *sbrk(int count) {
	void *ptr = heap + heap_sz;
	if ((heap_sz + count) <= HEAP_SIZE) {
		heap_sz += count;
		return ptr;
	} else {
		return (void *)-1;
	}
}
