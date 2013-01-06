/**
 * Copyright (c) 1986, 1988, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *  (c) UNIX System Laboratories, Inc.
 * Copyright (c) 2013
 *  Benjamin Vanheuverzwijn <bvanheu@gmail.com>
 *
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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

#include <stdarg.h>

#include "../arch/io.h"
#include "klib.h"

typedef unsigned int size_t;
typedef int ssize_t;

typedef unsigned long long u_quad_t;
typedef long long quad_t;

typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

/*
 * defines
 */

/* flags for kprintf */
#define TOCONS		0x01	/* to the console */
#define TOTTY		0x02	/* to the process' tty */
#define TOLOG		0x04	/* to the kernel message buffer */
#define TOBUFONLY	0x08	/* to the buffer (only) [for snprintf] */
#define TODDB		0x10	/* to ddb console */
#define TOCOUNT		0x20	/* act like [v]snprintf */

/* max size buffer kprintf needs to print quad_t [size in base 8 + \0] */
#define KPRINTF_BUFSIZE		(sizeof(quad_t) * 8 / 3 + 2)


/*
 * local prototypes
 */

int	 kprintf(const char *, int, void *, char *, va_list);

/*
 * normal kernel printf functions: printf, vprintf, snprintf
 */

/*
 * printf: print a message to the console and the log
 */
int
printf(const char *fmt, ...)
{
	va_list ap;
	int retval;

	va_start(ap, fmt);
	retval = kprintf(fmt, TOCONS | TOLOG, NULL, NULL, ap);
	va_end(ap);

	return(retval);
}

/*
 * vprintf: print a message to the console and the log [already have a
 *	va_list]
 */

int
vprintf(const char *fmt, va_list ap)
{
	int retval;

	retval = kprintf(fmt, TOCONS | TOLOG, NULL, NULL, ap);

	return (retval);
}

/*
 * snprintf: print a message to a buffer
 */
int
snprintf(char *buf, size_t size, const char *fmt, ...)
{
	int retval;
	va_list ap;
	char *p;

	p = buf + size - 1;
	if (size < 1)
		p = buf;
	va_start(ap, fmt);
	retval = kprintf(fmt, TOBUFONLY | TOCOUNT, &p, buf, ap);
	va_end(ap);
	if (size > 0)
		*(p) = 0;	/* null terminate */
	return(retval);
}

/*
 * vsnprintf: print a message to a buffer [already have va_alist]
 */
int
vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
	int retval;
	char *p;

	p = buf + size - 1;
	if (size < 1)
		p = buf;
	retval = kprintf(fmt, TOBUFONLY | TOCOUNT, &p, buf, ap);
	if (size > 0)
		*(p) = 0;	/* null terminate */
	return(retval);
}

/*
 * kprintf: scaled down version of printf(3).
 *
 * this version based on vfprintf() from libc which was derived from
 * software contributed to Berkeley by Chris Torek.
 *
 * The additional format %b is supported to decode error registers.
 * Its usage is:
 *
 *	printf("reg=%b\n", regval, "<base><arg>*");
 *
 * where <base> is the output base expressed as a control character, e.g.
 * \10 gives octal; \20 gives hex.  Each arg is a sequence of characters,
 * the first of which gives the bit number to be inspected (origin 1), and
 * the next characters (up to a control character, i.e. a character <= 32),
 * give the name of the register.  Thus:
 *
 *	kprintf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 *
 * would produce output:
 *
 *	reg=3<BITTWO,BITONE>
 *
 * To support larger integers (> 32 bits), %b formatting will also accept
 * control characters in the region 0x80 - 0xff.  0x80 refers to bit 0,
 * 0x81 refers to bit 1, and so on.  The equivalent string to the above is:
 *
 *	kprintf("reg=%b\n", 3, "\10\201BITTWO\200BITONE\n");
 *
 * and would produce the same output.
 *
 * Like the rest of printf, %b can be prefixed to handle various size
 * modifiers, eg. %b is for "int", %lb is for "long", and %llb supports
 * "long long".
 *
 * This code is large and complicated...
 */

/*
 * macros for converting digits to letters and vice versa
 */
#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')

/*
 * flags used during conversion.
 */
#define	ALT		0x001		/* alternate form */
#define	HEXPREFIX	0x002		/* add 0x or 0X prefix */
#define	LADJUST		0x004		/* left adjustment */
#define	LONGDBL		0x008		/* long double; unimplemented */
#define	LONGINT		0x010		/* long integer */
#define	QUADINT		0x020		/* quad integer */
#define	SHORTINT	0x040		/* short integer */
#define	ZEROPAD		0x080		/* zero (as opposed to blank) pad */
#define FPT		0x100		/* Floating point number */
#define SIZEINT		0x200		/* (signed) size_t */

	/*
	 * To extend shorts properly, we need both signed and unsigned
	 * argument extraction methods.
	 */
#define	SARG() \
	(flags&QUADINT ? va_arg(ap, quad_t) : \
	    flags&LONGINT ? va_arg(ap, long) : \
	    flags&SIZEINT ? va_arg(ap, ssize_t) : \
	    flags&SHORTINT ? (long)(short)va_arg(ap, int) : \
	    (long)va_arg(ap, int))
#define	UARG() \
	(flags&QUADINT ? va_arg(ap, u_quad_t) : \
	    flags&LONGINT ? va_arg(ap, u_long) : \
	    flags&SIZEINT ? va_arg(ap, size_t) : \
	    flags&SHORTINT ? (u_long)(u_short)va_arg(ap, int) : \
	    (u_long)va_arg(ap, u_int))

#define KPRINTF_PUTCHAR(C) do {					\
	int chr = (C);							\
	ret += 1;							\
	if (oflags & TOBUFONLY) {					\
		if ((vp != NULL) && (sbuf == tailp)) {			\
			if (!(oflags & TOCOUNT))				\
				goto overflow;				\
		} else							\
			*sbuf++ = chr;					\
	} else {							\
		io_putchar(chr);                \
	}								\
} while(0)

int
kprintf(const char *fmt0, int oflags, void *vp, char *sbuf, va_list ap)
{
	char *fmt;		/* format string */
	int ch;			/* character from fmt */
	int n;			/* handy integer (short term usage) */
	char *cp = NULL;	/* handy char pointer (short term usage) */
	int flags;		/* flags as above */
	int ret;		/* return value accumulator */
	int width;		/* width from format (%8d), or 0 */
	int prec;		/* precision from format (%.3d), or -1 */
	char sign;		/* sign prefix (' ', '+', '-', or \0) */

	u_quad_t _uquad;	/* integer arguments %[diouxX] */
	enum { OCT, DEC, HEX } base;/* base for [diouxX] conversion */
	int dprec;		/* a copy of prec if [diouxX], 0 otherwise */
	int realsz;		/* field size expanded by dprec */
	int size = 0;		/* size of converted field or string */
	char *xdigs = NULL;	/* digits for [xX] conversion */
	char buf[KPRINTF_BUFSIZE]; /* space for %c, %[diouxX] */
	char *tailp = NULL;	/* tail pointer for snprintf */

	if ((oflags & TOBUFONLY) && (vp != NULL))
		tailp = *(char **)vp;

	fmt = (char *)fmt0;
	ret = 0;

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		while (*fmt != '%' && *fmt) {
			KPRINTF_PUTCHAR(*fmt++);
		}
		if (*fmt == 0)
			goto done;

		fmt++;		/* skip over '%' */

		flags = 0;
		dprec = 0;
		width = 0;
		prec = -1;
		sign = '\0';

rflag:		ch = *fmt++;
reswitch:	switch (ch) {
		/* XXX: non-standard '%b' format */
		case 'b': {
			char *b, *z;
			int tmp;
			_uquad = UARG();
			b = va_arg(ap, char *);
			if (*b == 8)
				snprintf(buf, sizeof buf, "%llo", _uquad);
			else if (*b == 10)
				snprintf(buf, sizeof buf, "%lld", _uquad);
			else if (*b == 16)
				snprintf(buf, sizeof buf, "%llx", _uquad);
			else
				break;
			b++;

			z = buf;
			while (*z) {
				KPRINTF_PUTCHAR(*z++);
			}

			if (_uquad) {
				tmp = 0;
				while ((n = *b++) != 0) {
					if (n & 0x80)
						n &= 0x7f;
					else if (n <= ' ')
						n = n - 1;
					if (_uquad & (1LL << n)) {
						KPRINTF_PUTCHAR(tmp ? ',':'<');
						while (*b > ' ' &&
						    (*b & 0x80) == 0) {
							KPRINTF_PUTCHAR(*b);
							b++;
						}
						tmp = 1;
					} else {
						while (*b > ' ' &&
						    (*b & 0x80) == 0)
							b++;
					}
				}
				if (tmp) {
					KPRINTF_PUTCHAR('>');
				}
			}
			continue;	/* no output */
		}

		case ' ':
			/*
			 * ``If the space and + flags both appear, the space
			 * flag will be ignored.''
			 *	-- ANSI X3J11
			 */
			if (!sign)
				sign = ' ';
			goto rflag;
		case '#':
			flags |= ALT;
			goto rflag;
		case '*':
			/*
			 * ``A negative field width argument is taken as a
			 * - flag followed by a positive field width.''
			 *	-- ANSI X3J11
			 * They don't exclude field widths read from args.
			 */
			if ((width = va_arg(ap, int)) >= 0)
				goto rflag;
			width = -width;
			/* FALLTHROUGH */
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '+':
			sign = '+';
			goto rflag;
		case '.':
			if ((ch = *fmt++) == '*') {
				n = va_arg(ap, int);
				prec = n < 0 ? -1 : n;
				goto rflag;
			}
			n = 0;
			while (is_digit(ch)) {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			}
			prec = n < 0 ? -1 : n;
			goto reswitch;
		case '0':
			/*
			 * ``Note that 0 is taken as a flag, not as the
			 * beginning of a field width.''
			 *	-- ANSI X3J11
			 */
			flags |= ZEROPAD;
			goto rflag;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 0;
			do {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			} while (is_digit(ch));
			width = n;
			goto reswitch;
		case 'h':
			flags |= SHORTINT;
			goto rflag;
		case 'l':
			if (*fmt == 'l') {
				fmt++;
				flags |= QUADINT;
			} else {
				flags |= LONGINT;
			}
			goto rflag;
		case 'q':
			flags |= QUADINT;
			goto rflag;
		case 'z':
			flags |= SIZEINT;
			goto rflag;
		case 'c':
			*(cp = buf) = va_arg(ap, int);
			size = 1;
			sign = '\0';
			break;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			_uquad = SARG();
			if ((quad_t)_uquad < 0) {
				_uquad = -_uquad;
				sign = '-';
			}
			base = DEC;
			goto number;
		case 'n':
			if (flags & QUADINT)
				*va_arg(ap, quad_t *) = ret;
			else if (flags & LONGINT)
				*va_arg(ap, long *) = ret;
			else if (flags & SHORTINT)
				*va_arg(ap, short *) = ret;
			else if (flags & SIZEINT)
				*va_arg(ap, ssize_t *) = ret;
			else
				*va_arg(ap, int *) = ret;
			continue;	/* no output */
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			_uquad = UARG();
			base = OCT;
			goto nosign;
		case 'p':
			/*
			 * ``The argument shall be a pointer to void.  The
			 * value of the pointer is converted to a sequence
			 * of printable characters, in an implementation-
			 * defined manner.''
			 *	-- ANSI X3J11
			 */
			/* NOSTRICT */
			_uquad = (u_long)va_arg(ap, void *);
			base = HEX;
			xdigs = "0123456789abcdef";
			flags |= HEXPREFIX;
			ch = 'x';
			goto nosign;
		case 's':
			if ((cp = va_arg(ap, char *)) == NULL)
				cp = "(null)";
			if (prec >= 0) {
				/*
				 * can't use strlen; can only look for the
				 * NUL in the first `prec' characters, and
				 * strlen() will go further.
				 */
				char *p = memchr(cp, 0, prec);

				if (p != NULL) {
					size = p - cp;
					if (size > prec)
						size = prec;
				} else
					size = prec;
			} else
				size = strlen(cp);
			sign = '\0';
			break;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
			_uquad = UARG();
			base = DEC;
			goto nosign;
		case 'X':
			xdigs = "0123456789ABCDEF";
			goto hex;
		case 'x':
			xdigs = "0123456789abcdef";
hex:			_uquad = UARG();
			base = HEX;
			/* leading 0x/X only if non-zero */
			if (flags & ALT && _uquad != 0)
				flags |= HEXPREFIX;

			/* unsigned conversions */
nosign:			sign = '\0';
			/*
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI X3J11
			 */
number:			if ((dprec = prec) >= 0)
				flags &= ~ZEROPAD;

			/*
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI X3J11
			 */
			cp = buf + KPRINTF_BUFSIZE;
			if (_uquad != 0 || prec != 0) {
				/*
				 * Unsigned mod is hard, and unsigned mod
				 * by a constant is easier than that by
				 * a variable; hence this switch.
				 */
				switch (base) {
				case OCT:
					do {
						*--cp = to_char(_uquad & 7);
						_uquad >>= 3;
					} while (_uquad);
					/* handle octal leading 0 */
					if (flags & ALT && *cp != '0')
						*--cp = '0';
					break;

				case DEC:
					/* many numbers are 1 digit */
					while (_uquad >= 10) {
						*--cp = to_char(_uquad % 10);
						_uquad /= 10;
					}
					*--cp = to_char(_uquad);
					break;

				case HEX:
					do {
						*--cp = xdigs[_uquad & 15];
						_uquad >>= 4;
					} while (_uquad);
					break;

				default:
					cp = "bug in kprintf: bad base";
					size = strlen(cp);
					goto skipsize;
				}
			}
			size = buf + KPRINTF_BUFSIZE - cp;
		skipsize:
			break;
		default:	/* "%?" prints ?, unless ? is NUL */
			if (ch == '\0')
				goto done;
			/* pretend it was %c with argument ch */
			cp = buf;
			*cp = ch;
			size = 1;
			sign = '\0';
			break;
		}

		/*
		 * All reasonable formats wind up here.  At this point, `cp'
		 * points to a string which (if not flags&LADJUST) should be
		 * padded out to `width' places.  If flags&ZEROPAD, it should
		 * first be prefixed by any sign or other prefix; otherwise,
		 * it should be blank padded before the prefix is emitted.
		 * After any left-hand padding and prefixing, emit zeroes
		 * required by a decimal [diouxX] precision, then print the
		 * string proper, then emit zeroes required by any leftover
		 * floating precision; finally, if LADJUST, pad with blanks.
		 *
		 * Compute actual size, so we know how much to pad.
		 * size excludes decimal prec; realsz includes it.
		 */
		realsz = dprec > size ? dprec : size;
		if (sign)
			realsz++;
		else if (flags & HEXPREFIX)
			realsz+= 2;

		/* right-adjusting blank padding */
		if ((flags & (LADJUST|ZEROPAD)) == 0) {
			n = width - realsz;
			while (n-- > 0)
				KPRINTF_PUTCHAR(' ');
		}

		/* prefix */
		if (sign) {
			KPRINTF_PUTCHAR(sign);
		} else if (flags & HEXPREFIX) {
			KPRINTF_PUTCHAR('0');
			KPRINTF_PUTCHAR(ch);
		}

		/* right-adjusting zero padding */
		if ((flags & (LADJUST|ZEROPAD)) == ZEROPAD) {
			n = width - realsz;
			while (n-- > 0)
				KPRINTF_PUTCHAR('0');
		}

		/* leading zeroes from decimal precision */
		n = dprec - size;
		while (n-- > 0)
			KPRINTF_PUTCHAR('0');

		/* the string or number proper */
		while (size--)
			KPRINTF_PUTCHAR(*cp++);
		/* left-adjusting padding (always blank) */
		if (flags & LADJUST) {
			n = width - realsz;
			while (n-- > 0)
				KPRINTF_PUTCHAR(' ');
		}
	}

done:
	if ((oflags & TOBUFONLY) && (vp != NULL))
		*(char **)vp = sbuf;
overflow:
	return (ret);
	/* NOTREACHED */
}

