#include "../nanokernel/errno.c"

/*
 * Define _k_neg_eagain for use in assembly files as errno.h is
 * not assembly language safe.
 */
const int _k_neg_eagain = -EAGAIN;
