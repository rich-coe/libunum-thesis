#include <stdio.h>
#include "unum.h"

void
factorial(SORN *s, int f)
{
    SORN tmp;
    int i;

    uemp(s);
    uint(s, 1, 1);

    for (i = f; i > 1; i--) {
        uemp(&tmp);
        uint(&tmp, i, i);
        umul(s, &tmp);
    }
}

int
main(void)
{
    SORN e, tmp;
    int i;

    uemp(&e);
    uint(&e, 1, 1);
    uout(&e);
    putchar('\n');

    for (i = 1; i <= 20; i++) {
        factorial(&tmp, i);
        uinv(&tmp);
        uadd(&e, &tmp);
        uout(&e);
        putchar('\n');
    }
    return 0;
}
