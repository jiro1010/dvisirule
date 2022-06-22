/*
 * Copyright (C) 2012-2022 Jiro Senju
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this package.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#elif defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#else
/* What can I do? */
#endif

#include <stddef.h>

#ifdef HAVE_STRING_H
#include <string.h>
#else
/* What can I do? */
#endif

enum dvi_instruction {
	SETCHAR0,
	SETCHAR127 = 127,
	SET1,		// c[1]
	SET2,		// c[2]
	SET3,		// c[3]
	SET4,		// c[4]
	SETRULE,	// a[4] b[4]
	PUT1,		// c[1]
	PUT2,		// c[2]
	PUT3,		// c[3]
	PUT4,		// c[4]
	PUTRULE,	// a[4] b[4]
	NOP,
	BOP,		// c0[4] c1[4]...c9[4] p[4]
	EOP,
	PUSH,
	POP,
	RIGHT1,		// b[1]
	RIGHT2,		// b[2]
	RIGHT3,		// b[3]
	RIGHT4,		// b[4]
	W0,
	W1,		// b[1]
	W2,		// b[2]
	W3,		// b[3]
	W4,		// b[4]
	X0,
	X1,		// b[1]
	X2,		// b[2]
	X3,		// b[3]
	X4,		// b[4]
	DOWN1,		// a[1]
	DOWN2,		// a[2]
	DOWN3,		// a[3]
	DOWN4,		// a[4]
	Y0,
	Y1,		// a[1]
	Y2,		// a[2]
	Y3,		// a[3]
	Y4,		// a[4]
	Z0,
	Z1,		// a[1]
	Z2,		// a[2]
	Z3,		// a[3]
	Z4,		// a[4]
	FNTNUM0,
	FNTNUM63 = FNTNUM0 + 63,
	FNT1,		// k[1]
	FNT2,		// k[2]
	FNT3,		// k[3]
	FNT4,		// k[4]
	XXX1,		// k[1] x[k]
	XXX2,		// k[2] x[k]
	XXX3,		// k[3] x[k]
	XXX4,		// k[4] x[k]
	FNTDEF1,	// k[1] c[4] s[4] d[4] a[1] l[1] n[a+l]
	FNTDEF2,	// k[2] c[4] s[4] d[4] a[1] l[1] n[a+l]
	FNTDEF3,	// k[3] c[4] s[4] d[4] a[1] l[1] n[a+l]
	FNTDEF4,	// k[4] c[4] s[4] d[4] a[1] l[1] n[a+l]
	PRE,		// i[1] num[4] den[4] mag[4] k[1] x[k]
	POST,		// p[4] num[4] den[4] mag[4] l[4] u[4] s[2] t[2]
	POSTPOST,	// q[4] i[1] 223 (more than 4 bytes)
	DIR = 255	// d[1] (ptex)
};

static inline uint16_t two(void *p)
{
	uint16_t v;
	ptrdiff_t a = (ptrdiff_t)p;

	if (a & 1)
		memcpy(&v, p, sizeof(v));
	else
		v = *(uint16_t *)p;
	return ntohs(v);
}

static inline uint32_t three(void *p)
{
	uint16_t u16;
	uint8_t u8, *q;

	u16 = two(p);
	p += 2;
	q = p;
	u8 = *q;
	return (u16 << 8) | u8;
}

static inline uint32_t four(void *p)
{
	uint32_t v;
	ptrdiff_t a = (ptrdiff_t)p;

	if (a & 3)
		memcpy(&v, p, sizeof(v));
	else
		v = *(uint32_t *)p;
	return ntohl(v);
}
