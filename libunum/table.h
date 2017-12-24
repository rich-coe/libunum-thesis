#include "unum.h"

struct _unumrange {
    unum a;
    unum b;
};

struct _unum {
    double val;
    char *name;
};

extern struct _unum unums[];
extern struct _unumrange addtable[];
extern struct _unumrange multable[];
extern struct _unumrange logtable[];
