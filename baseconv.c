#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BASE_MIN 2
#define BASE_MAX 94
#define P_MAX (((size_t)-1 >> (sizeof(size_t) << 2))+1)
#define BUF_SIZE 256

struct bigint_s {
	size_t m;
	size_t *p;
};
typedef struct bigint_s bigint_t;

struct bigint_divide_s {
	bigint_t *q;
	bigint_t *r;
};
typedef struct bigint_divide_s bigint_divide_t;

#include "baseconv.h"
bigint_t *str_to_bigint(const char *, const char *);
char *bigint_to_str(const bigint_t *, const char *);
size_t *check_digits(const char *, size_t);
bigint_divide_t *bigint_int_divide(const bigint_t *, size_t);
bigint_t *bigint_int_add(const bigint_t *, size_t);
bigint_t *bigints_add(const bigint_t *, const bigint_t *);
bigint_t *bigint_int_multiply(const bigint_t *, size_t);
bigint_t *bigints_subtract(const bigint_t *, const bigint_t *);
void hilo_add(size_t, size_t *, size_t *);
void hilo_subtract(size_t, size_t, size_t *, size_t *);
bigint_t *bigint_copy(const bigint_t *);
int bigint_reduce(bigint_t *);
bigint_t *bigint_create(size_t);
void bigint_destroy(bigint_t *);
bigint_divide_t *bigint_divide_create(const bigint_t *);
void bigint_divide_destroy(bigint_divide_t *);
void reverse_str(char *, size_t);

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
size_t len = strlen(str), base = strlen(digs), *vals, i;
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
	for (i = 0; i < len && vals[str[i]-rdigs[0]] < base; i++);
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
size_t base = strlen(digs), *vals, i;
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
		str[i-1] = digs[tdiv->r->p[0]];
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

size_t *check_digits(const char *digs, size_t base) {
size_t *vals = malloc(sizeof(size_t)*BASE_MAX), i;
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

bigint_divide_t *bigint_int_divide(const bigint_t *a, size_t b) {
size_t i;
bigint_t *h, *tmp;
bigint_divide_t *d = bigint_divide_create(a);
	if (!d) {
		return NULL;
	}
	while (d->r->m > 1 || d->r->p[0] >= b) {
		if (d->r->p[d->r->m-1] < b) {
			h = bigint_create(d->r->m-1);
			if (!h) {
				bigint_divide_destroy(d);
				return NULL;
			}
			h->p[h->m-1] = d->r->p[d->r->m-1]*P_MAX+d->r->p[d->r->m-2];
		}
		else {
			if (!(h = bigint_create(d->r->m))) {
				bigint_divide_destroy(d);
				return NULL;
			}
			h->p[h->m-1] = d->r->p[d->r->m-1];
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
			bigint_divide_destroy(d);
			return NULL;
		}
		tmp = h;
		h = bigint_int_multiply(h, b);
		bigint_destroy(tmp);
		if (!h) {
			bigint_divide_destroy(d);
			return NULL;
		}
		tmp = d->r;
		d->r = bigints_subtract(d->r, h);
		bigint_destroy(tmp);
		bigint_destroy(h);
		if (!d->r) {
			bigint_divide_destroy(d);
			return NULL;
		}
	}
	return d;
}

bigint_t *bigint_int_add(const bigint_t *a, size_t b) {
size_t i;
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
size_t i;
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

bigint_t *bigint_int_multiply(const bigint_t *a, size_t b) {
size_t i;
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
size_t c, i;
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

void hilo_add(size_t v, size_t *hi, size_t *lo) {
	if (v < P_MAX) {
		*hi = 0;
		*lo = v;
	}
	else {
		*hi = v/P_MAX;
		*lo = v%P_MAX;
	}
}

void hilo_subtract(size_t v1, size_t v2, size_t *hi, size_t *lo) {
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
size_t i;
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
size_t i, *tmp;
	for (i = r->m-1; i > 0 && !r->p[i]; i--);
	if (i < r->m-1) {
		r->m = i+1;
		tmp = realloc(r->p, sizeof(size_t)*r->m);
		if (!tmp) {
			bigint_destroy(r);
			return EXIT_FAILURE;
		}
		r->p = tmp;
	}
	return EXIT_SUCCESS;
}

bigint_t *bigint_create(size_t m) {
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
	r->p = malloc(sizeof(size_t)*m);
	if (!r->p) {
		free(r);
		return NULL;
	}
	return r;
}

void bigint_destroy(bigint_t *r) {
	if (r->p) free(r->p);
	free(r);
}

bigint_divide_t *bigint_divide_create(const bigint_t *a) {
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
	d->r = bigint_copy(a);
	if (!d->r) {
		bigint_destroy(d->q);
		free(d);
		return NULL;
	}
	return d;
}

void bigint_divide_destroy(bigint_divide_t *d) {
	bigint_destroy(d->r);
	bigint_destroy(d->q);
	free(d);
}

void reverse_str(char *str, size_t len) {
size_t i, j;
char tchr;
	for (i = 0, j = len-1; i < j; i++, j--) {
		tchr = str[i];
		str[i] = str[j];
		str[j] = tchr;
	}
}