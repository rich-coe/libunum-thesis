#include <float.h>
#include <math.h>
#include <stdio.h>

#include "table.h"

#undef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))

static size_t
blur(double val)
{
    size_t i;
    if (isinf(val)) {                                   /* infinity is infinity */
        return NUMUNUMS / 2;
    }

    for (i = 0; i < NUMUNUMS; i++) {                    /* equality within relative epsilon */
        if (isfinite(unums[i].val) && fabs(unums[i].val - val) <= DBL_EPSILON * MAX(fabs(unums[i].val), fabs(val))) {
            return i;
        }

        /* in range */
        if (isnan(unums[i].val) && val < unums[UCLAMP(i, +1)].val && val > unums[UCLAMP(i, -1)].val) {
            return i;
        }
    }

    /* negative or positive range outshot */
    return (val < 0) ? (NUMUNUMS / 2 + 1) : (val > 0) ? (NUMUNUMS / 2 - 1) : 0;
}

static void
_sornaddrange(SORN *s, unum lower, unum upper)
{
    unum u;
    size_t i, j;

    for (int first = 1, u = lower; u != UCLAMP(upper, +1) || (first && lower == UCLAMP(upper, +1)); u = UCLAMP(u, +1)) {
        first = 0;
        i = u / (sizeof(*s->data) * 8);
        j = u % (sizeof(*s->data) * 8);
        s->data[i] |= (1 << (sizeof(*s->data) * 8 - 1 - j));
    }
}

static unum
_unumnegate(unum u)
{
    return UCLAMP(NUMUNUMS, -u);
}

static unum
_unuminvert(unum u)
{
    return _unumnegate(UCLAMP(u, +(NUMUNUMS / 2)));
}

static unum
_unumabs(unum u)
{
    return (u > NUMUNUMS / 2) ? _unumnegate(u) : u;
}

static void
_sornop(SORN *a, SORN *b, struct _unumrange table[], unum (*mod)(unum))
{
    unum u, v, low, upp;
    size_t i, j, m, n;
    static SORN res;

    for (i = 0; i < sizeof(res.data); i++) {                /* empty result SORN */
        res.data[i] = 0;
    }

    for (i = 0; i < sizeof(a->data); i++) {
        for (j = 0; j < sizeof(*a->data) * 8; j++) {
            if (!(a->data[i] & (1 << (sizeof(*a->data) * 8 - 1 - j)))) {
                continue;
            }
            /* unum u = (so * i + j) is in the first set */
            u = sizeof(*a->data) * 8 * i + j;
            for (m = 0; m < sizeof(b->data); m++) {
                for (n = 0; n < sizeof(*b->data) * 8; n++) {
                    if (!(b->data[m] & (1 << (sizeof(*b->data) * 8 - 1 - n)))) {
                        continue;
                    }

                    /* unum v = (so * m + n) is in the second set */
                    v = sizeof(*b->data) * 8 * m + n;

                    /* compare struct pointers to  identify dependent arguments
                     * and in this case only do pairwise operations
                     */

                    if (a == b && u != v)
                        continue;

                    /* apply an optional modifier after the dependency check */
                    if (mod) {
                        v = mod(v);
                    }

                    /* get bounds from table; according to gauß 1 + 2 + 3 ... + n = (n * (n + 1)) / 2,
                     * used to traverse triangle array
                     */
                   if (u <= v) {
                        low = table[((size_t)v * (v + 1)) / 2 + u].a;
                        upp = table[((size_t)v * (v + 1)) / 2 + u].b;
                    } else {
                        low = table[((size_t)u * (u + 1)) / 2 + v].a;
                        upp = table[((size_t)u * (u + 1)) /2 + v].b;
                    }
                    _sornaddrange(&res, low, upp);
                }
            }
        }
    }

    /* write result to first operand */
    for (i = 0; i < sizeof(a->data); i++) {
        a->data[i] = res.data[i];
    }
}

static void
_sornmod(SORN *s, unum (mod)(unum))
{
    SORN res;
    unum u;
    size_t i, j, k, l;

    for (i = 0; i < sizeof(res.data); i++) {
        res.data[i] = 0;
    }

    for (i = 0; i < sizeof(s->data); i++) {
        for (j = 0; j < sizeof(*s->data) * 8; j++) {
            if (!(s->data[i] & (1 << (sizeof(*s->data) * 8 - 1 - j)))) {
                continue;
            }

            /* unum u = (so * i + j) is in the set */
            u = sizeof(*s->data) * 8 * i + j;
            u = mod(u);

            k = u / (sizeof(*s->data) * 8);
            l = u % (sizeof(*s->data) * 8);
            res.data[k] |= (1 << (sizeof(*res.data) * 8 - 1 - l));
        }
    }

    /* write result to operand */
    for (i = 0; i < sizeof(s->data); i++) {
        s->data[i] = res.data[i];
    }
}

void
uadd(SORN *a, SORN *b)
{
    _sornop(a, b, addtable, NULL);
}

void
usub(SORN *a, SORN *b)
{
    _sornop(a, b, addtable, _unumnegate);
}

void
umul(SORN *a, SORN *b)
{
    _sornop(a, b, multable, NULL);
}

void
udiv(SORN *a, SORN *b)
{
    _sornop(a, b, multable, _unuminvert);
}

void
uneg(SORN *s)
{
    _sornmod(s, _unumnegate);
}

void
uinv(SORN *s)
{
    _sornmod(s, _unuminvert);
}

void
uabs(SORN *s)
{
    _sornmod(s, _unumabs);
}

void
ulog(SORN *s)
{
    unum u;
    size_t i, j;
    static SORN res;

    for (i = 0; i < sizeof(res.data); i++) {                    /* empty result SORN */
        res.data[i] = 0;
    }

    for (i = 0; i < sizeof(s->data); i++) {
        for (j = 0; j < sizeof(*s->data) * 8; j++) {
            if (!(s->data[i] & (1 << (sizeof(*s->data) * 8 - 1 - j)))) {
                continue;
            }

            /* unum u = (so * i + j) is in the set */
            u = sizeof(*s->data) * 8 * i + j;

            /* is SORN negative? not defined */
            if (u > NUMUNUMS / 2) {
                _sornaddrange(&res, 0, NUMUNUMS - 1);
                goto done;
            }

            /* read the table and apply ranges */
            _sornaddrange(&res, logtable[u].a, logtable[u].b);
        }
    }

done:
    /* write result to operand */
    for (i = 0; i < sizeof(s->data); i++) {
        s->data[i] = res.data[i];
    }
}

void
uemp(SORN *s)
{
    size_t i;
    for (i = 0; i < sizeof(s->data); i++) {
        s->data[i] = 0;
    }
}
void
uset(SORN *a, SORN *b)
{
    size_t i;
    for (i = 0; i < sizeof(a->data); i++) {
        a->data[i] = b->data[i];
    }
}
void
ucut(SORN *a, SORN *b)
{
    size_t i;
    for (i = 0; i < sizeof(a->data); i++) {
        a->data[i] = a->data[i] & b->data[i];
    }
}
void
uuni(SORN *a, SORN *b)
{
    size_t i;
    for (i = 0; i < sizeof(a->data); i++) {
        a->data[i] = a->data[i] | b->data[i];
    }
}
int
uequ(SORN *a, SORN *b)
{
    size_t i;
    for (i = 0; i < sizeof(a->data); i++) {
        if (a->data[i] != b->data[i]) {
            return 0;
        }
    }
    return 1;
}

int
usup(SORN *a, SORN *b)
{
    ucut(a, b);
    return uequ(a, b);
}

void
uint(SORN *s, double lower, double upper)
{
    _sornaddrange(s, blur(lower), blur(upper));
}

void
uout(SORN *s)
{
    unum loopstart, u;
    size_t i, j;
    int active, insorn, loop2run;

    loop2run = 0;
    for (active = 0, i = sizeof(s->data) / 2; i < sizeof(s->data); i++) {
loop1start:
        for (j = 0; j < sizeof(*s->data) * 8; j++) {
            u = sizeof(*s->data) * 8 * i + j;
            insorn = s->data[i] & (1 << (sizeof(*s->data) * 8 - 1 - j));

            if (!active && insorn) {                    /* print the opening of a closed subset */
                active = 1;
                if (unums[u].name) {
                    printf("[%s,", unums[u].name);
                } else {
                    printf("(%s,", unums[UCLAMP(u, -1)].name);
                }
            } else if (active && !insorn) {             /* print the closing of a closed subset */
                active = 0;
                if (unums[UCLAMP(u, -1)].name) {
                    printf("%s]␣", unums[UCLAMP(u, -1)].name);
                } else {
                    printf("%s)␣", unums[u].name);
                }
            }
        }
        if (loop2run) {
            goto loop2end;
        }
    }
    loop2run = 1;
    for (i = 0; i < sizeof(s->data) / 2; i++) {
        goto loop1start;
loop2end:
        ;
    }

    if (active) {
        printf("\u221E)");
    }
}
