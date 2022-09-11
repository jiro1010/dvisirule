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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
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

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <err.h>
#include <errno.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
/* What can I do? */
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
/* What can I do? */
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
/* What can I do? */
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#else
/* What can I do? */
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
/* What can I do? */
#endif

#if HAVE_MALLOC != 1 || HAVE_MEMSET != 1 || HAVE_MMAP != 1 || HAVE_REALLOC != 1
#error malloc/memset/mmap/realloc unimplemented?
#endif

#include "dvi.h"

/* ---------------------------------------------------------------------- */

#if 0
#define Dpri(fmt, ...)	fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define Dpri(fmt, ...)	do {} while (0)
#endif

#define Errx(cond, fmt, args...) \
	if (cond) \
		errx(errno, "%s:%d: " fmt, __func__, __LINE__, ##args)
#define Err(cond, str) Errx(cond, "%s %m", str)

#define ExpandStep	25

struct dvifile {
	int fd;
	unsigned char *p;
} src, dst;

/* ---------------------------------------------------------------------- */

/* single SETRULE/PUTRULE with its color and coordinate */
union path {
	unsigned char a[64];
	struct {
		int n, color;
		unsigned char p[];
	};
};

#define PathCk(path) \
	Errx(path->n >= sizeof(*path) - sizeof(path->n), "%d >= %lu", \
		path->n, (unsigned long)sizeof(*path) - sizeof(path->n));

static void p_push(union path *path)
{
	unsigned char *p;

	p = path->p + path->n;
	*p = PUSH;
	path->n++;
	PathCk(path);
}

static void p_pop(union path *path)
{
	unsigned char *p;

	p = path->p + path->n;
	*p = POP;
	path->n++;
	PathCk(path);
}

static int dvi_move(unsigned char *p, unsigned char op, int v)
{
	int n;
	union {
		int v;
		unsigned char a[sizeof(int)];
	} u;

	Dpri("%p, 0x%x, %d\n", p, op, v);
	if (!v)
		return 0;

	u.v = htonl(v);
	n = 5;
	*p++ = op + 3;
	memcpy(p, u.a + 0, 4);

	return n;
}

static void p_moveto(union path *path, int h, int v)
{
	int n;

	n = dvi_move(path->p + path->n, RIGHT1, h);
	path->n += n;
	PathCk(path);
	n = dvi_move(path->p + path->n, DOWN1, v);
	path->n += n;
	PathCk(path);
}

static void p_pushcolor(union path *path, unsigned int offset)
{
	//XXX1
	static unsigned char cmd[] = "\xef\x11" "color push gray 0";
	unsigned char *p, *s;
	unsigned int len;

	path->color = path->n;
	p = path->p + path->n;
	if (offset) {
		s = src.p + offset;
		len = 1 + 1 + src.p[offset + 1];
	} else {
		s = cmd;
		len = sizeof(cmd) - 1;
	}
	path->n += len;
	PathCk(path);
	memcpy(p, s, len);
}

static void p_rule(union path *path, unsigned int offset)
{
	unsigned char *p;
	const unsigned int len = 1 + 4 + 4;

	p = path->p + path->n;
	path->n += len;
	PathCk(path);
	/* no setrule since it moves the current-point */
	*p++ = PUTRULE;
	memcpy(p, src.p + offset + 1, len - 1);
}

static void p_popcolor(union path *path)
{
	//XXX1
	static char cmd[] = "\xef\x09" "color pop";
	const unsigned int len = sizeof(cmd) - 1;
	unsigned char *p;

	p = path->p + path->n;
	path->n += len;
	PathCk(path);
	memcpy(p, cmd, len);
}

/* ---------------------------------------------------------------------- */

/* all SETRULE/PUTRULE in a page */
struct page {
	unsigned int total, cur;
	int h, v;
	/* to be qsort-able, use array instead of list */
	union path **path;
};

static int page_init(struct page *page)
{
	page->total = ExpandStep;
	page->cur = 0;
	page->path = malloc(sizeof(page->path) * page->total);
	return !!page->path;
}

static void page_reinit(struct page *page)
{
	page->cur = 0;
}

static int path_append(struct page *page, union path *path)
{
	union path **p;

	if (page->cur + 1 >= page->total) {
		/* expand */
		page->total += ExpandStep;
		p = realloc(page->path, sizeof(page->path) * page->total);
		Err(!p, "realloc");
		page->path = p;
	}
	page->path[page->cur++] = path;

	return 0;
}

static int color_sort(const void *__a, const void *__b)
{
	const union path **_a = (const union path **)__a,
		**_b = (const union path **)__b;
	const union path *a = *_a, *b = *_b;
	char ca[32], cb[32];
	int l;

	l = a->p[a->color + 1];
	memcpy(ca, a->p + a->color + 2, l);
	ca[l] = 0;

	l = b->p[b->color + 1];
	memcpy(cb, b->p + b->color + 2, l);
	cb[l] = 0;

	l = strcmp(ca, cb);
	if (l > 0)
		l = -1;
	else if (l < 0)
		l = 1;
	return l;
}

static void page_write(int fd, struct page *page)
{
	int i, n;
	union path *path, origin;
	ssize_t ssz;

	origin.n = 0;
	p_push(&origin);
	p_moveto(&origin, -page->h, -page->v);
	ssz = write(fd, origin.p, origin.n);
	Err(ssz != origin.n, "write");

	n = page->cur;
	qsort(page->path, n, sizeof(*page->path), color_sort);
	for (i = 0; i < n; i++) {
		path = page->path[i];
		ssz = write(fd, path->p, path->n);
		Err(ssz != path->n, "write");
	}

	origin.n = 0;
	p_pop(&origin);
	ssz = write(fd, origin.p, origin.n);
	Err(ssz != origin.n, "write");
}

/* ---------------------------------------------------------------------- */

static void postamble(unsigned int last, unsigned int ofbop, unsigned int srcsz)
{
	unsigned int post, prev;
	ssize_t ssz;
	int n;

	Errx(src.p[last] != POST, "last %u, 0x%x expects 0x%x",
	     last, src.p[last], POST);
	Dpri("last %u, 0x%x\n", last, last);
	post = lseek(dst.fd, 0, SEEK_CUR);
	ssz = write(dst.fd, src.p + last, 1);
	Err(ssz != 1, "write");
	last++;
	ofbop = htonl(ofbop);
	ssz = write(dst.fd, &ofbop, 4);
	Err(ssz != 4, "write");
	last += 4;
	ssz = write(dst.fd, src.p + last, 4 * 6);
	Err(ssz != 4 * 6, "write");
	last += 4 * 6;

	prev = last;
	while (src.p[last] != POSTPOST) {
		n = 0;
		switch (src.p[last]) {
		case FNTDEF4:
			n++;
		case FNTDEF3:
			n++;
		case FNTDEF2:
			n++;
		case FNTDEF1:
			n++;
			break;
		default:
			Errx(src.p[last], "0x%x at %u(0x%x)",
			     src.p[last], last, last);
		}
		Dpri("last %u(0x%x), n %d\n", last, last, n);
		n += 4 * 3 + 1 + 1;
		n += src.p[last + n] + 1;
		Dpri("n %d\n", n);
		last += n;
	}
	ssz = write(dst.fd, src.p + prev, last - prev);
	Err(ssz != last - prev, "write");

	/* post_post */
	Dpri("last %u, 0x%x\n", last, last);
	Errx(src.p[last] != POSTPOST, "last %u, 0x%x expects 0x%x",
	     last, src.p[last], POSTPOST);
	ssz = write(dst.fd, src.p + last, 1);
	Err(ssz != 1, "write");
	last++;
	post = htonl(post);
	ssz = write(dst.fd, &post, 4);
	Err(ssz != 4, "write");
	last += 4;
	ssz = write(dst.fd, src.p + last, srcsz - last);
	Err(ssz != srcsz - last, "write");
}

/*
 * in dvi, all offsets are represented in 32 bits.
 * it means no 'long', 'long long', or 'loff_t' are necessary.
 */

int main(int argc, char *argv[])
{
	int e, n, h, v;
	unsigned int height, width, hpx, wpx, last;
	/* offsets */
	unsigned int ofbop, ofcolor, ofrule, ofeop, ofline;
	float gray;
	char cmd[BUFSIZ], op[BUFSIZ], colorpush[64], *cmdp, c;
	struct stat st;
	union path *path;
	struct page page;
	ssize_t ssz;

	src.fd = open(argv[1], O_RDWR);
	Err(src.fd < 0, argv[1]);

	last = 0;
	e = fstat(src.fd, &st);
	Err(e, argv[1]);
	src.p = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE,
		     src.fd, 0);
	Err(src.p == MAP_FAILED, argv[1]);

	dst.fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	Err(dst.fd < 0, argv[2]);

	page_init(&page);
	while (1) {
		/* read the output of marker.awk */
		cmdp = fgets(cmd, sizeof(cmd), stdin);
		if (!cmdp)
			break;

		if (*cmdp == 'h') {
			/*
			 * depends on the TeX version or variant?
			 * h=-31 v=176 <offset>:color_push_gray_0
			 * <offset>: putrule height 26214, width 2290850
			 * (2x146 pixels)
			 */
			n = sscanf(cmdp, "h=%d v=%d "
				   "%u:color_push_%s "
				   "%u: %s height %u, width %u "
				   "(%ux%u pixels)",
				   &h, &v,
				   &ofcolor, colorpush,
				   &ofrule, op,
				   &height, &width,
				   &hpx, &wpx);
			if (n != 10) {
				n = sscanf(cmdp, "h=%d v=%d "
					   "%u:pdf:bcolor [%f] "
					   "%u: %s height %u, width %u "
					   "(%ux%u pixels)",
					   &h, &v,
					   &ofcolor, &gray,
					   &ofrule, op,
					   &height, &width,
					   &hpx, &wpx);
				Errx(n != 10, "n %d, %s", n, cmdp);
			}

			path = malloc(sizeof(*path));
			Err(!path, "malloc");

			path->n = 0;
			p_push(path);
			p_moveto(path, h, v);
			p_pushcolor(path, ofcolor);
			p_rule(path, ofrule);
			p_popcolor(path);
			p_pop(path);
			path_append(&page, path);
		} else if (!strncmp(cmdp, "bop", 3)) {
			/* bop <offset>: beginning of page 1 */
			n = sscanf(cmdp, "bop %u: beginning %s",
				   &ofbop, op);
			Errx(n != 2, "n %d, %s", n, cmdp);
		} else if (!strncmp(cmdp, "bol", 3)
			   || !strncmp(cmdp, "eol", 3)) {
			/* bol <offset>: xxx ' sirule BOL' */
			/* eol <offset>: xxx ' sirule EOL' */
			n = sscanf(cmdp, "%col %u: %s",
				   &c, &ofline, op);
			Errx(n != 3, "n %d, %s", n, cmdp);
			memset(src.p + ofline, NOP, src.p[ofline + 1] + 2);
		} else {
			/* <offset>: eop  h=0 v=2206 */
			n = sscanf(cmdp, "%u: eop  h=%d v=%d",
				   &ofeop, &page.h, &page.v);
			Errx(n != 3, "n %d, %s", n, cmdp);

			Dpri("last %u\n", last);
			ssz = write(dst.fd, src.p + last, ofeop);
			Errx(ssz != ofeop, "write, %p, %u, %u",
			     src.p, last, ofeop);
			last = ofeop;
			Dpri("last %u, 0x%x\n", last, last);
			page_write(dst.fd, &page);
			ssz = write(dst.fd, src.p + last, 1);
			Err(ssz != 1, "write");
			last++;
			page_reinit(&page);
		}
	}

	postamble(last, ofbop, st.st_size);

	return 0;
}
