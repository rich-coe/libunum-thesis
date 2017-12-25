#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>

#undef LEN
#define LEN(x) sizeof(x) / sizeof(*x)

#undef UCLAMP
#define UCLAMP(i, off) (((((off < 0) && (i) < -off) ? \
            numunums - ((-off - (i)) % numunums) : \
            ((off > 0) && (i) + off > numunums - 1) ? \
            ((i) + off % numunums) % numunums : (i) + off)) % numunums)

#undef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#undef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))

struct _unum {
    double val;
    char *name;
};

struct _unumrange {
    size_t low;
    size_t upp;
};

struct _latticep {
    char *name;
    double val;
};

static void
printunums(FILE *fp, struct _unum *unum, size_t numunums)
{
    size_t i;
    fputs("\nstruct _unum unums[] = {\n", fp);
    for (i = 0; i < numunums; i++) {
        if (isnan(unum[i].val) && !unum[i].name) {
            fprintf(fp, "\t{ NAN, NULL },\n");
        } else if (isinf(unum[i].val)) {
            fprintf(fp, "\t{ INFINITY, \"%s\" },\n", unum[i].name);
        } else {
            fprintf(fp, "\t{ %f, \"%s\" },\n", unum[i].val, unum[i].name);
        }
    }
    fputs("};\n", fp);
}

size_t
blur(double val, struct _unum *unum, size_t numunums)
{
    size_t i;
    if (isinf(val)) {                           /* infinity is infinity */
        return numunums / 2;
    }

    for (i = 0; i < numunums; i++) {            /* equality within relative epsilon */
        if (isfinite(unum[i].val) && fabs(unum[i].val - val) <= DBL_EPSILON * MAX(fabs(unum[i].val), fabs(val))) {
            return i;
        }
        /* in range */
        if (isnan(unum[i].val) && val < unum[UCLAMP(i, +1)].val && val > unum[UCLAMP(i, -1)].val) {
            return i;
        }
    }

    /* negative or positive range outshot */
    return (val < 0) ? (numunums / 2 + 1) : (val > 0) ?  (numunums / 2 - 1) : 0;
}

void
add(size_t a, size_t b, struct _unumrange *res, struct _unum *unum, size_t numunums)
{
    double av, bv, aupp, alow, bupp, blow;

    av = unum[a].val;
    bv = unum[b].val;
    if (isnan(av) && isnan(bv)) {                               /* a interval, b interval */
        aupp = unum[UCLAMP(a, +1)].val;
        alow = unum[UCLAMP(a, -1)].val;
        bupp = unum[UCLAMP(b, +1)].val;
        blow = unum[UCLAMP(b, -1)].val;
        if ((isinf(alow) && isinf(aupp)) || (isinf(blow) && isinf(bupp))) {     /* all real numbers */
            res->low = numunums / 2 + 1;
            res->upp = numunums / 2 - 1;
            return;
        } else if (isinf(alow) && isinf(blow)) {                /* (iffy, aupp + bupp) */
            res->low = numunums / 2 + 1;
            fesetround(FE_UPWARD);
            res->upp = blur(aupp + bupp, unum, numunums);
        } else if (isinf(aupp) && isinf(bupp)) {                /* (alow + blow, iffy) */
            fesetround(FE_DOWNWARD);
            res->low = blur(alow + blow, unum, numunums);
            res->upp = numunums / 2 + 1;
        } else {                                                /* (alow + blow, aupp + bupp) */
            fesetround(FE_DOWNWARD);
            res->low = blur(alow + blow, unum, numunums);
            fesetround(FE_UPWARD);
                res->upp = blur(aupp + bupp, unum, numunums);
        }
    } else if (!isnan(av) && !isnan(bv)) {                      /* a point, b point */
        if (isinf(av) && isinf(bv)) {                           /* all extended real numbers */
            res->low = 0;
            res->upp = numunums - 1;
            return;
        } else {
            fesetround(FE_DOWNWARD);
            res->low = blur(av + bv, unum, numunums);
            fesetround(FE_UPWARD);
            res->upp = blur(av + bv, unum, numunums);
        }
    } else if (!isnan(av) && isnan(bv)) {                       /* a point, b interval */
        bupp = unum[UCLAMP(b, +1)].val;
        blow = unum[UCLAMP(b, -1)].val;
        if (isinf(av)) {                                        /* all extended real numbers */
            res->low = 0;
            res->upp = numunums - 1;
            return;
        } else {
            fesetround(FE_DOWNWARD);
            res->low = blur(av + blow, unum, numunums);
            fesetround(FE_UPWARD);
            res->upp = blur(av + bupp, unum, numunums);
        }
    } else if (!isnan(bv) && isnan(av)) {                       /* a interval, b point */
        add(b, a, res, unum, numunums);
        return;
    }

    if (isnan(av) || isnan(bv)) {
        /* we had an open interval in our calculation and need to check if res->upp or res->low
         * are a point. If this is the case, we have to round it down to respect the openness
         * of the real interval
         */
        if (!isnan(unum[res->low].val)) {
            res->low = UCLAMP(res->low, +1);
        }
        if (!isnan(unum[res->upp].val)) {
            res->upp = UCLAMP(res->upp, -1);
        }
    }
}

void
mul(size_t a, size_t b, struct _unumrange *res, struct _unum *unum, size_t numunums)
{
    double av, bv, aupp, alow, bupp, blow;

    av = unum[a].val;
    bv = unum[b].val;
    if (isnan(av) && isnan(bv)) {                       /* a interval, b interval */
        aupp = unum[UCLAMP(a, +1)].val;
        alow = unum[UCLAMP(a, -1)].val;
        bupp = unum[UCLAMP(b, +1)].val;
        blow = unum[UCLAMP(b, -1)].val;
        if ((isinf(alow) && isinf(aupp)) || (isinf(blow) && isinf(bupp))) {     /* all real numbers */
            res->low = numunums / 2 + 1;
            res->upp = numunums / 2 - 1;
            return;
        } else if (isinf(alow) && isinf(blow)) {
            if (aupp <= 0 && bupp <= 0) {               /* (aupp * bupp, iffy) */
                fesetround(FE_DOWNWARD);
                res->low = blur(aupp * bupp, unum, numunums);
                res->upp = numunums / 2 - 1;
            } else {                                    /* all real numbers */
                res->low = numunums / 2 + 1;
                res->upp = numunums / 2 - 1;
                return;
            }
        } else if (isinf(aupp) && isinf(bupp)) {
            if (alow >= 0 && blow >= 0) {               /* (alow * blow, iffy) */
                fesetround(FE_DOWNWARD);
                res->low = blur(alow * blow, unum, numunums);
                res->upp = numunums / 2 - 1;
            } else {                                    /* all real numbers */
                res->low = numunums / 2 + 1;
                res->upp = numunums / 2 - 1;
                return;
            }
        } else if (isinf(alow) && isinf(bupp)) {
            if (aupp <= 0 && blow >= 0) {               /* (iffy, aupp * blow) */
                res->low = numunums / 2 + 1;
                fesetround(FE_UPWARD);
                res->upp = blur(aupp * blow, unum, numunums);
            } else {                                    /* all real numbers */
                res->low = numunums / 2 + 1;
                res->upp = numunums / 2 - 1;
                return;
            }
        } else if (isinf(aupp) && isinf(blow)) {
            mul(b, a, res, unum, numunums);
            return;
        } else if (isinf(alow)) {
            if (blow >= 0) {                            /* (iffy, MAX(aupp * blow, aupp * bupp) */
                res->low = numunums / 2 + 1;
                fesetround(FE_UPWARD);
                res->upp = blur(MAX(aupp * blow, aupp * bupp), unum, numunums);
            } else if (bupp <= 0) {                     /* (MIN(aupp * blow, aupp * bupp), * iffy) */
                fesetround(FE_DOWNWARD);
                res->low = blur(MIN(aupp * blow, aupp * bupp), unum, numunums);
                res->upp = numunums / 2 - 1;
            } else {                                    /* all real numbers */
                res->low = numunums / 2 + 1;
                res->upp = numunums / 2 - 1;
                return;
            }
        } else if (isinf(aupp)) {
            if (blow >= 0) {                            /* (MIN(alow * blow, aupp * bupp), iffy) */
                fesetround(FE_DOWNWARD);
                res->low = blur(MIN(alow * blow, alow * bupp), unum, numunums);
                res->upp = numunums / 2 - 1;
            } else if (bupp <= 0) {                     /* (iffy, MAX(alow * blow, alow * bupp) */
                res->low = numunums / 2 + 1;
                fesetround(FE_UPWARD);
                res->upp = blur(MAX(alow * blow, alow * bupp), unum, numunums);
            } else {                                    /* all real numbers */
                res->low = numunums / 2 + 1;
                res->upp = numunums / 2 - 1;
                return;
            }
        } else if (isinf(blow) || isinf(bupp)) {
            mul(b, a, res, unum, numunums);
        } else {                                        /* (MIN(C), MAX(C)) */
            fesetround(FE_DOWNWARD);
            res->low = blur(MIN(MIN(alow * blow, alow * bupp), MIN(aupp * blow, aupp * bupp)), unum, numunums);
            fesetround(FE_UPWARD);
            res->upp = blur(MAX(MAX(alow * blow, alow * bupp), MAX(aupp * blow, aupp * bupp)), unum, numunums);
        }
    } else if (!isnan(av) && !isnan(bv)) {              /* a point, b point */
        if ((isinf(av) && (fabs(bv) <= DBL_EPSILON * fabs(bv) || isinf(bv))) 
            || (isinf(bv) && (fabs(av) <= DBL_EPSILON * fabs(av) || isinf(av)))) {  /* all extended real numbers */
            res->low = 0;
            res->upp = numunums - 1;
            return;
        } else {
            fesetround(FE_DOWNWARD);
            res->low = blur(av * bv, unum, numunums);
            fesetround(FE_UPWARD);
            res->upp = blur(av * bv, unum, numunums);
        }
    } else if (!isnan(av) && isnan(bv)) {               /* a point, b interval */
        bupp = unum[UCLAMP(b, +1)].val;
        blow = unum[UCLAMP(b, -1)].val;
        if (isinf(av)) {
            if (isinf(blow)) {
                if (bupp < 0) {                         /* infinity */
                    res->low = numunums / 2;
                    res->upp = numunums / 2;
                    return;
                } else {                                /* all extended real numbers */
                    res->low = 0;
                    res->upp = numunums - 1;
                    return;
                }
            } else if (isinf(bupp)) {
                if (blow > 0) {                         /* infinity */
                    res->low = numunums / 2;
                    res->upp = numunums / 2;
                    return;
                } else {                                /* all extended real numbers */
                    res->low = 0;
                    res->upp = numunums - 1;
                    return;
                }
            } else if ((blow < 0) == (bupp < 0)) {      /* infinity */
                res->low = numunums / 2;
                res->upp = numunums / 2;
                return;
            } else {                                    /* all extended real numbers */
                res->low = 0;
                res->upp = numunums - 1;
                return;
            }
        } else {                        /* (MIN(av * blow, av * bupp), MAX(av * blow, av * bupp)) */
            fesetround(FE_DOWNWARD);
            res->low = blur(MIN(av * blow, av * bupp), unum, numunums);
            fesetround(FE_UPWARD);
            res->upp = blur(MAX(av * blow, av * bupp), unum, numunums);
        }
    } else if (isnan(av) && !isnan(bv)) {               /* a interval, b point */
        mul(b, a, res, unum, numunums);
        return;
    }

    if (isnan(av) || isnan(bv)) {
        /* we had an open interval in our calculation and need to check if res->upp or res->low
         * are a point. If this is the case, we have to round it down to respect the openness
         * of the real interval
         */
        if (!isnan(unum[res->low].val)) {
            res->low = UCLAMP(res->low, +1);
        }
        if (!isnan(unum[res->upp].val)) {
            res->upp = UCLAMP(res->upp, -1);
        }
    }
}

static void
gentable(FILE *fp, char *name, void (*f)(size_t, size_t, struct _unumrange *, struct _unum *, size_t), struct _unum *unum, size_t numunums)
{
    struct _unumrange res;
    size_t s, z;

    fprintf(fp, "\nstruct _unumrange %stable[] = {\n", name);

    printf("gentable %s %d unums\n", name, numunums);

    for (z = 0; z < numunums; z++) {
        putc('\t', fp);
        for (s = 0; s <= z; s++) {
            f(s, z, &res, unum, numunums);
            fprintf(fp, "%s{ %zd, %zd },", s ? " " : "", res.low, res.upp);
        }
        fputs("\n", fp);
    }
    fputs("};\n", fp);
}

void
ulog(size_t u, struct _unumrange *res, struct _unum *unum, size_t numunums)
{
    double uv, ulow, uupp;

    uv = unum[u].val;
    if (isnan(uv)) {
        ulow = unum[UCLAMP(u, -1)].val;
        uupp = unum[UCLAMP(u, +1)].val;
        res->low = blur(log(ulow), unum, numunums);
        res->upp = blur(log(uupp), unum, numunums);
    } else {
        res->low = res->upp = blur(log(uv), unum, numunums);
    }
}

static void
genfunctable(FILE *fp, char *name, void (*f)(size_t, struct _unumrange *, struct _unum *, size_t), struct _unum *unum, size_t numunums)
{
    struct _unumrange res;
    size_t u;

    fprintf(fp, "\nstruct _unumrange %stable[] = {\n", name);

    for (u = 0; u <= numunums / 2; u++) {
        f(u, &res, unum, numunums);
        fprintf(fp, "\t{ %zd, %zd },\n", res.low, res.upp);
    }
    fputs("};\n", fp);
}

static void
genunums(struct _latticep *lattice, size_t latticesize, struct _unum *unum, size_t numunums)
{
    size_t off;
    ssize_t i;

    off = 0;

    /* 0 */
    unum[off].val = 0.0;
    unum[off].name = "0";
    off++;
    unum[off].val = NAN;
    unum[off].name = NULL;
    off++;

    /* (0,1) */
    for (i = latticesize - 1; i >= 0; i--, off++) {
        unum[off].val = 1 / lattice[i].val;
        if (lattice[i].name[0] == '/') {
            unum[off].name = lattice[i].name + 1;
        } else {
            /* add '/' prefix */
            if (!(unum[off].name = malloc(strlen(lattice[i].name) + 2))) {
                fprintf(stderr, "out of memory\n");
                exit(1);
            }
            strcpy(unum[off].name + 1, lattice[i].name);
            unum[off].name[0] = '/';
        }
        off++;
        unum[off].val = NAN;
        unum[off].name = NULL;
    }

    /* 1 */
    unum[off].val = 1.0;
    unum[off].name = "1";
    off++;
    unum[off].val = NAN;
    unum[off].name = NULL;
    off++;

    /* (1,INF) */
    for (i = 0; i < latticesize; i++, off++) {
        unum[off].val = lattice[i].val;
        unum[off].name = lattice[i].name;
        off++;
        unum[off].val = NAN;
        unum[off].name = NULL;
    }

    /* INF */
    unum[off].val = INFINITY;
    unum[off].name = "\u221E";
    off++;
    unum[off].val = NAN;
    unum[off].name = NULL;
    off++;

    /* (INF,-1) */
    for (i = latticesize - 1; i >= 0; i--, off++) {
        unum[off].val = -lattice[i].val;
        if (!(unum[off].name = malloc(strlen(lattice[i].name) + 2))) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        strcpy(unum[off].name + 1, lattice[i].name);
        unum[off].name[0] = '-';
        off++;
        unum[off].val = NAN;
        unum[off].name = NULL;
    }

    /* -1 */
    unum[off].val = -1.0;
    unum[off].name = "-1";
    off++;
    unum[off].val = NAN;
    unum[off].name = NULL;
    off++;

    /* (-1, 0) */
    for (i = 0; i < latticesize; i++, off++) {
        unum[off].val = - 1 / lattice[i].val;
        if (lattice[i].name[0] == '/') {
            if (!(unum[off].name = strdup(lattice[i].name))) {
                fprintf(stderr, "out of memory\n");
                exit(1);
            }
            unum[off].name[0] = '-';
        } else {
            /* add '-/' prefix */
            if (!(unum[off].name = malloc(strlen(lattice[i].name) + 3))) {
                fprintf(stderr, "out of memory\n");
                exit(1);
            }
            strcpy(unum[off].name + 2, lattice[i].name);
            unum[off].name[0] = '-';
            unum[off].name[1] = '/';
        }
        off++;
        unum[off].val = NAN;
        unum[off].name = NULL;
    }
}

void
gendeclattice(struct _latticep **lattice, size_t *latticesize, double maximum, int sigdigs)
{
    size_t i, maxlen;
    double c1, c2, curmax;
    char *fmt = "%.*f";

    /*
     * Check prerequisites
     */
    if (sigdigs == 0) {
        fprintf(stderr, "invalid number of " "significant digits\n");
    }

    if ((*latticesize == 0) == isinf(maximum)) {
        fprintf(stderr, "gendeclattice: accepting only one parameter besides number of significant digits\n");
        exit(1);
    }

    c1 = pow(10, sigdigs) - pow(10, sigdigs - 1);
    c2 = pow(10, -(sigdigs - 1));

    if (*latticesize == 0) {            /* calculate lattice size until maximum is contained */
        for (curmax = 0; curmax < maximum; (*latticesize)++) {
            curmax = (1 + c2 * (*latticesize % (size_t)c1)) * pow(10, floor(*latticesize / c1));
        }
    } else {                            /* isinf(maximum) -> calculate maximum */
        maximum = (1 + c2 * (*latticesize % (size_t)c1)) * pow(10, floor(*latticesize / c1));
    }

    /*
     * Generate lattice
     */
    if (!(*lattice = malloc(sizeof(struct _latticep) * *latticesize))) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    maxlen = snprintf(NULL, 0, fmt, sigdigs - 1, maximum) + 1;
    for (i = 0; i < *latticesize; i++) {
        (*lattice)[i].val = (1 + c2 * ((i + 1) % (size_t)c1)) * pow(10, floor((i + 1) / c1));
        if (!((*lattice)[i].name = malloc(maxlen))) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        snprintf((*lattice)[i].name, maxlen, fmt,
        sigdigs - 1, (*lattice)[i].val);
    }
}

int
main(void)
{
    struct _unum *unum;
    struct _latticep *lattice;
    size_t latticebits, latticesize, numunums;
    ssize_t i;
    int bits;

    /* Generate lattice */
    latticesize = (1 << (UBITS - 3)) - 1;
    gendeclattice(&lattice, &latticesize, INFINITY, DIGITS);

    FILE *fp = fopen("unum.h", "w");

    if (NULL == fp) {
        fprintf(stderr, "cannot open '%s' for writing\n", "unum.h");
        exit(1);
    }

    /*
     * Print unum.h includes
     */
    fprintf(fp, "#include <math.h>\n#include <stddef.h>\n#include <stdint.h>\n\n");
    /*
     * Determine number of effective bits used
     */
    struct {
        int bits;
        char *type;
    } types[] = {
        { 8, "uint8_t" },
        { 16, "uint16_t" },
        { 32, "uint32_t" },
        { 64, "uint64_t" },
    };

    numunums = 8 * (latticesize + 1);
    for (bits = 1; bits <= types[LEN(types) - 1].bits; bits++) {
        if (numunums == (1 << bits))
            break;
    }

    if (bits > types[LEN(types) - 1].bits) {
        fprintf(stderr, "invalid number of latticepoints\n");
        return 1;
    }

    /*
     * Determine type needed to store the unum
     */
    for (i = 0; i < LEN(types); i++) {
        if (types[i].bits >= bits)
            break;
    }

    if (i == LEN(types)) {
        fprintf(stderr, "cannot fit bits into systemtypes\n");
        return 1;
    }

    /*
     * Print list of preliminary unum.h definitions
     */
    fprintf(fp, "typedef %s unum;\n#define ULEN %d\n#define NUMUNUMS %zd\n", types[i].type, bits, numunums);
    fprintf(fp, "#define UCLAMP(i, off) (((((off < 0) && (i) <"
        "-off) ? \\\n\tNUMUNUMS - ((-off - (i)) %% NUMUNUMS) :"
        "\\\n\t((off > 0) && (i) + off > NUMUNUMS - 1) ? \\\n\t"
        "((i) + off %% NUMUNUMS) %% NUMUNUMS : (i) + off)) %% "
        "NUMUNUMS)\n\n");
     
    fprintf(fp, "typedef struct {\n\tuint8_t data[%d];\n} SORN;\n", (1 << bits) / 8);

    fprintf(fp, "\nvoid uadd(SORN *, SORN *);\n"
            "void usub(SORN *, SORN *);\n"
            "void umul(SORN *, SORN *);\n"
            "void udiv(SORN *, SORN *);\n"
            "void uneg(SORN *);\n"
            "void uinv(SORN *);\n"
            "void uabs(SORN *);\n\n"
            "void ulog(SORN *);\n\n"
            "void uemp(SORN *);\n"
            "void uset(SORN *, SORN *);\n"
            "void ucut(SORN *, SORN *);\n"
            "void uuni(SORN *, SORN *);\n"
            "int uequ(SORN *, SORN *);\n"
            "int usup(SORN *, SORN *);\n\n"
            "void uint(SORN *, double, double);\n"
            "void uout(SORN *);\n");

    fclose(fp);

    fp = fopen("table.c", "w");

    if (NULL == fp) {
        fprintf(stderr, "cannot open '%s' for writing\n", "table.c");
        exit(1);
    }

    /*
     * Generate unums
     */
    if (!(unum = malloc(sizeof(struct _unum) * numunums))) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }
    genunums(lattice, latticesize, unum, numunums);

    /*
     * Print table.c includes
     */
    fprintf(fp, "#include \"table.h\"\n");

    /*
     * Print list of unums
     */
    printunums(fp, unum, numunums);

    /*
     * Generate and print tables
     */
    gentable(fp, "add", add, unum, numunums);
    gentable(fp, "mul", mul, unum, numunums);

    /*
     * Generate function tables
     */
    genfunctable(fp, "log", ulog, unum, numunums);
    fclose(fp);
    return 0;
}
