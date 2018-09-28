/*    $NetBSD: fnmatch.c,v 1.26 2014/10/12 22:32:33 christos Exp $    */

/*
 * Copyright (c) 1989, 1993, 1994
 *    The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Guido van Rossum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Function fnmatch() as specified in POSIX 1003.2-1992, section B.6.
 * Compares a filename or pathname to a pattern.
 */

#include <ctype.h>
#include "fnmatch.h"
#include <string.h>

#define EOS    '\0'

static inline int foldcase(int ch, int flags)
{

    if ((flags & FNM_CASEFOLD) != 0 && isupper(ch))
        return tolower(ch);
    return ch;
}

#define    FOLDCASE(ch, flags)    foldcase((unsigned char)(ch), (flags))

static const char * rangematch(const char *pattern, int test, int flags)
{
    int negate, ok, need;
    char c, c2;

    if (pattern == NULL)
    {
        return NULL;
    }

    /*
     * A bracket expression starting with an unquoted circumflex
     * character produces unspecified results (IEEE 1003.2-1992,
     * 3.13.2).  This implementation treats it like '!', for
     * consistency with the regular expression syntax.
     * J.T. Conklin (conklin@ngai.kaleida.com)
     */
    if ((negate = (*pattern == '!' || *pattern == '^')) != 0)
        ++pattern;
    
    need = 1;
    for (ok = 0; (c = FOLDCASE(*pattern++, flags)) != ']' || need;) {
        need = 0;
        if (c == '/')
            return (void *)-1;
        if (c == '\\' && !(flags & FNM_NOESCAPE))
            c = FOLDCASE(*pattern++, flags);
        if (c == EOS)
            return NULL;
        if (*pattern == '-' 
            && (c2 = FOLDCASE(*(pattern + 1), flags)) != EOS &&
                c2 != ']') {
            pattern += 2;
            if (c2 == '\\' && !(flags & FNM_NOESCAPE))
                c2 = FOLDCASE(*pattern++, flags);
            if (c2 == EOS)
                return NULL;
            if (c <= test && test <= c2)
                ok = 1;
        } else if (c == test)
            ok = 1;
    }
    return ok == negate ? NULL : pattern;
}


static int fnmatchx(const char *pattern, const char *string, int flags, size_t recursion)
{
    const char *stringstart, *r;
    char c, test;

    if ((pattern == NULL) || (string == NULL))
    {
        return FNM_NOMATCH;
    }

    if (recursion-- == 0)
        return FNM_NORES;

    for (stringstart = string;;) {
        switch (c = FOLDCASE(*pattern++, flags)) {
        case EOS:
            if ((flags & FNM_LEADING_DIR) && *string == '/')
                return 0;
            return *string == EOS ? 0 : FNM_NOMATCH;
        case '?':
            if (*string == EOS)
                return FNM_NOMATCH;
            if (*string == '/' && (flags & FNM_PATHNAME))
                return FNM_NOMATCH;
            if (*string == '.' && (flags & FNM_PERIOD) &&
                (string == stringstart ||
                ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
                return FNM_NOMATCH;
            ++string;
            break;
        case '*':
            c = FOLDCASE(*pattern, flags);
            /* Collapse multiple stars. */
            while (c == '*')
                c = FOLDCASE(*++pattern, flags);

            if (*string == '.' && (flags & FNM_PERIOD) &&
                (string == stringstart ||
                ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
                return FNM_NOMATCH;

            /* Optimize for pattern with * at end or before /. */
            if (c == EOS) {
                if (flags & FNM_PATHNAME)
                    return (flags & FNM_LEADING_DIR) ||
                        strchr(string, '/') == NULL ?
                        0 : FNM_NOMATCH;
                else
                    return 0;
            } else if (c == '/' && flags & FNM_PATHNAME) {
                if ((string = strchr(string, '/')) == NULL)
                    return FNM_NOMATCH;
                break;
            }

            /* General case, use recursion. */
            while ((test = FOLDCASE(*string, flags)) != EOS) {
                int e;
                switch ((e = fnmatchx(pattern, string,
                    flags & ~FNM_PERIOD, recursion))) {
                case FNM_NOMATCH:
                    break;
                default:
                    return e;
                }
                if (test == '/' && flags & FNM_PATHNAME)
                    break;
                ++string;
            }
            return FNM_NOMATCH;
        case '[':
            if (*string == EOS)
                return FNM_NOMATCH;
            if (*string == '/' && flags & FNM_PATHNAME)
                return FNM_NOMATCH;
            if ((r = rangematch(pattern,
                FOLDCASE(*string, flags), flags)) == NULL)
                return FNM_NOMATCH;
            if (r == (void *)-1) {
                if (*string != '[')
                    return FNM_NOMATCH;
            } else
                pattern = r;
            ++string;
            break;
        case '\\':
            if (!(flags & FNM_NOESCAPE)) {
                if ((c = FOLDCASE(*pattern++, flags)) == EOS) {
                    c = '\0';
                    --pattern;
                }
            }
            /* FALLTHROUGH */
        default:
            if (c != FOLDCASE(*string++, flags))
                return FNM_NOMATCH;
            break;
        }
    }
    /* NOTREACHED */
}

int fnmatch(const char *pattern, const char *string, int flags)
{
    return fnmatchx(pattern, string, flags, 64);
}

