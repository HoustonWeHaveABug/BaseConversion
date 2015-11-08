#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BASE_MIN 2
#define BASE_MAX 94
#define P_MAX (((unsigned long)-1 >> (sizeof(unsigned long) << 2))+1)
#define BUF_SIZE 256

struct bigint_s {
	unsigned long m;
	unsigned long *p;
};
typedef struct bigint_s bigint_t;

struct bigint_divide_s {
	bigint_t *q;
	unsigned long r;
};
typedef struct bigint_divide_s bigint_divide_t;

#include "baseconv.h"
bigint_t *str_to_bigint(const char *, const char *);
char *bigint_to_str(const bigint_t *, const char *);
unsigned long *check_digits(const char *, unsigned long);
bigint_divide_t *bigint_int_divide(const bigint_t *, unsigned long);
bigint_t *bigint_int_add(const bigint_t *, unsigned long);
bigint_t *bigints_add(const bigint_t *, const bigint_t *);
bigint_t *bigint_int_multiply(const bigint_t *, unsigned long);
bigint_t *bigints_subtract(const bigint_t *, const bigint_t *);
void hilo_add(unsigned long, unsigned long *, unsigned long *);
void hilo_subtract(unsigned long, unsigned long, unsigned long *, unsigned long *);
bigint_t *bigint_copy(const bigint_t *);
int bigint_reduce(bigint_t *);
bigint_t *bigint_create(unsigned long);
void bigint_destroy(bigint_t *);
bigint_divide_t *bigint_divide_create(void);
void bigint_divide_destroy(bigint_divide_t *);
void reverse_str(char *, unsigned long);

const char *rdigs = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

char *base_convert(const char *istr, const char *idigs, const char *odigs) {
char *ostr;
bigint_t *val = str_to_bigint(istr, idigs);
	if (!val) {
		return NULL;
	}
	ostr = bigint_to_str(val, odigs);
	bigint_destroy(val);
	return ostr;
}

bigint_t *str_to_bigint(const char *str, const char *digs) {
unsigned long len = strlen(str), base = strlen(digs), *vals, i;
bigint_t *val, *tval;
	if (!len || !base) {
		errno = EINVAL;
		return NULL;
	}
	vals = check_digits(digs, base);
	if (!vals) {
		errno = EINVAL;
		return NULL;
	}
	for (i = 0; i < len && strchr(digs, str[i]); i++);
	if (i < len) {
		errno = EINVAL;
		free(vals);
		return NULL;
	}
	val = bigint_create(1UL);
	if (!val) {
		free(vals);
		return NULL;
	}
	val->p[0] = vals[str[0]-rdigs[0]];
	for (i = 1; i < len; i++) {
		tval = val;
		val = bigint_int_multiply(val, base);
		bigint_destroy(tval);
		if (!val) {
			free(vals);
			return NULL;
		}
		tval = val;
		val = bigint_int_add(val, vals[str[i]-rdigs[0]]);
		bigint_destroy(tval);
		if (!val) {
			free(vals);
			return NULL;
		}
	}
	free(vals);
	return val;
}

char *bigint_to_str(const bigint_t *val, const char *digs) {
unsigned long base = strlen(digs), *vals, i;
char *str, *tstr;
bigint_t *cval, *tval;
bigint_divide_t *tdiv;
	if (base < BASE_MIN) {
		errno = EINVAL;
		return NULL;
	}
	vals = check_digits(digs, base);
	if (!vals) {
		errno = EINVAL;
		return NULL;
	}
	free(vals);
	str = malloc(sizeof(char)*BUF_SIZE);
	if (!str) {
		return NULL;
	}
	cval = bigint_copy(val);
	if (!cval) {
		free(str);
		return NULL;
	}
	i = 0;
	do {
		i++;
		if (!(i%BUF_SIZE)) {
			tstr = realloc(str, sizeof(char)*(i+BUF_SIZE));
			if (!tstr) {
				bigint_destroy(cval);
				free(str);
				return NULL;
			}
			str = tstr;
		}
		tdiv = bigint_int_divide(cval, base);
		if (!tdiv) {
			bigint_destroy(cval);
			free(str);
			return NULL;
		}
		str[i-1] = digs[tdiv->r];
		tval = cval;
		cval = bigint_copy(tdiv->q);
		bigint_divide_destroy(tdiv);
		bigint_destroy(tval);
		if (!cval) {
			free(str);
			return NULL;
		}
	}
	while (cval->m > 1 || cval->p[0]);
	bigint_destroy(cval);
	str[i] = 0;
	reverse_str(str, i);
	return str;
}

unsigned long *check_digits(const char *digs, unsigned long base) {
unsigned long *vals = malloc(sizeof(unsigned long)*BASE_MAX), i;
	if (!vals) {
		return NULL;
	}
	for (i = 0; i < BASE_MAX; i++) {
		vals[i] = base;
	}
	for (i = 0; i < base; i++) {
		if (!strchr(rdigs, digs[i]) || vals[digs[i]-rdigs[0]] < base) {
			errno = EINVAL;
			free(vals);
			return NULL;
		}
		vals[digs[i]-rdigs[0]] = i;
	}
	return vals;
}

bigint_divide_t *bigint_int_divide(const bigint_t *a, unsigned long b) {
unsigned long i;
bigint_t *r, *h, *tmp;
bigint_divide_t *d = bigint_divide_create();
	if (!d) {
		return NULL;
	}
	r = bigint_copy(a);
	if (!r) {
		bigint_divide_destroy(d);
		return NULL;
	}
	while (r->m > 1 || r->p[0] >= b) {
		if (r->p[r->m-1] < b) {
			h = bigint_create(r->m-1);
			if (!h) {
				bigint_destroy(r);
				bigint_divide_destroy(d);
				return NULL;
			}
			h->p[h->m-1] = r->p[r->m-1]*P_MAX+r->p[r->m-2];
		}
		else {
			h = bigint_create(r->m);
			if (!h) {
				bigint_destroy(r);
				bigint_divide_destroy(d);
				return NULL;
			}
			h->p[h->m-1] = r->p[r->m-1];
		}
		h->p[h->m-1] /= b;
		for (i = 0; i < h->m-1; i++) {
			h->p[i] = 0;
		}
		tmp = d->q;
		d->q = bigints_add(d->q, h);
		bigint_destroy(tmp);
		if (!d->q) {
			bigint_destroy(h);
			bigint_destroy(r);
			bigint_divide_destroy(d);
			return NULL;
		}
		tmp = h;
		h = bigint_int_multiply(h, b);
		bigint_destroy(tmp);
		if (!h) {
			bigint_destroy(r);
			bigint_divide_destroy(d);
			return NULL;
		}
		tmp = r;
		r = bigints_subtract(r, h);
		bigint_destroy(tmp);
		bigint_destroy(h);
		if (!r) {
			bigint_divide_destroy(d);
			return NULL;
		}
	}
	d->r = r->p[0];
	bigint_destroy(r);
	return d;
}

bigint_t *bigint_int_add(const bigint_t *a, unsigned long b) {
unsigned long i;
bigint_t *r = bigint_create(a->m+1);
	if (!r) {
		return NULL;
	}
	hilo_add(a->p[0]+b, &r->p[1], &r->p[0]);
	for (i = 1; i < a->m; i++) {
		hilo_add(a->p[i]+r->p[i], &r->p[i+1], &r->p[i]);
	}
	return bigint_reduce(r) == EXIT_SUCCESS ? r:NULL;
}

bigint_t *bigints_add(const bigint_t *a, const bigint_t *b) {
unsigned long i;
const bigint_t *min, *max;
bigint_t *r;
	if (a->m < b->m) {
		min = a;
		max = b;
	}
	else {
		min = b;
		max = a;
	}
	r = bigint_create(max->m+1);
	if (!r) {
		return NULL;
	}
	hilo_add(a->p[0]+b->p[0], &r->p[1], &r->p[0]);
	for (i = 1; i < min->m; i++) {
		hilo_add(a->p[i]+b->p[i]+r->p[i], &r->p[i+1], &r->p[i]);
	}
	for (i = min->m; i < max->m; i++) {
		hilo_add(max->p[i]+r->p[i], &r->p[i+1], &r->p[i]);
	}
	return bigint_reduce(r) == EXIT_SUCCESS ? r:NULL;
}

bigint_t *bigint_int_multiply(const bigint_t *a, unsigned long b) {
unsigned long i;
bigint_t *r = bigint_create(a->m+1);
	if (!r) {
		return NULL;
	}
	hilo_add(a->p[0]*b, &r->p[1], &r->p[0]);
	for (i = 1; i < a->m; i++) {
		hilo_add(a->p[i]*b+r->p[i], &r->p[i+1], &r->p[i]);
	}
	return bigint_reduce(r) == EXIT_SUCCESS ? r:NULL;
}

bigint_t *bigints_subtract(const bigint_t *a, const bigint_t *b) {
unsigned long c, i;
bigint_t *r;
	if (a->m < b->m || (a->m == b->m && a->p[a->m-1] < b->p[b->m-1])) {
		errno = EINVAL;
		return NULL;
	}
	r = bigint_create(a->m);
	if (!r) {
		return NULL;
	}
	hilo_subtract(a->p[0], b->p[0], &c, &r->p[0]);
	for (i = 1; i < b->m; i++) {
		hilo_subtract(a->p[i], b->p[i]+c, &c, &r->p[i]);
	}
	for (i = b->m; i < a->m; i++) {
		hilo_subtract(a->p[i], c, &c, &r->p[i]);
	}
	return bigint_reduce(r) == EXIT_SUCCESS ? r:NULL;
}

void hilo_add(unsigned long v, unsigned long *hi, unsigned long *lo) {
	if (v < P_MAX) {
		*hi = 0;
		*lo = v;
	}
	else {
		*hi = v/P_MAX;
		*lo = v%P_MAX;
	}
}

void hilo_subtract(unsigned long v1, unsigned long v2, unsigned long *hi, unsigned long *lo) {
	if (v1 < v2) {
		*hi = 1;
		*lo = v1+P_MAX-v2;
	}
	else {
		*hi = 0;
		*lo = v1-v2;
	}
}

bigint_t *bigint_copy(const bigint_t *a) {
unsigned long i;
bigint_t *r = bigint_create(a->m);
	if (!r) {
		return NULL;
	}
	for (i = 0; i < r->m; i++) {
		r->p[i] = a->p[i];
	}
	return r;
}

int bigint_reduce(bigint_t *r) {
unsigned long i, *tmp;
	for (i = r->m-1; i > 0 && !r->p[i]; i--);
	if (i < r->m-1) {
		r->m = i+1;
		tmp = realloc(r->p, sizeof(unsigned long)*r->m);
		if (!tmp) {
			bigint_destroy(r);
			return EXIT_FAILURE;
		}
		r->p = tmp;
	}
	return EXIT_SUCCESS;
}

bigint_t *bigint_create(unsigned long m) {
bigint_t *r;
	if (m < 1) {
		errno = EINVAL;
		return NULL;
	}
	r = malloc(sizeof(bigint_t));
	if (!r) {
		return NULL;
	}
	r->m = m;
	r->p = malloc(sizeof(unsigned long)*m);
	if (!r->p) {
		free(r);
		return NULL;
	}
	return r;
}

void bigint_destroy(bigint_t *r) {
	if (r->p) {
		free(r->p);
	}
	free(r);
}

bigint_divide_t *bigint_divide_create(void) {
bigint_divide_t *d = malloc(sizeof(bigint_divide_t));
	if (!d) {
		return NULL;
	}
	d->q = bigint_create(1UL);
	if (!d->q) {
		free(d);
		return NULL;
	}
	d->q->p[0] = 0;
	return d;
}

void bigint_divide_destroy(bigint_divide_t *d) {
	bigint_destroy(d->q);
	free(d);
}

void reverse_str(char *str, unsigned long len) {
unsigned long i, j;
char tchr;
	for (i = 0, j = len-1; i < j; i++, j--) {
		tchr = str[i];
		str[i] = str[j];
		str[j] = tchr;
	}
}
