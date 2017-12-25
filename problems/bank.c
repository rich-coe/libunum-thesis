#include <math.h>
#include <stdio.h>
#include "unum.h"

int
main(void)
{
    SORN a, tmp;
    int y;

    uemp(&a);
    uint(&a, M_E - 1, M_E - 1);

    for (y = 1; y <= 25; y++) {
        uemp(&tmp);
        uint(&tmp, y, y);
        umul(&a, &tmp);

        uemp(&tmp);
        uint(&tmp, 1, 1);
        usub(&a, &tmp);

        printf("year␣%2d:␣", y);
        uout(&a);
        putchar('\n');
    }

    return 0;
}
