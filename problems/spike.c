#include <stdio.h>
#include "unum.h"

#define NUMPOINTS (10)
#define POLE (4.0/3.0)

int
main(void)
{
    SORN res, tmp;
    size_t i, j;
    unum pole, u;

    /* get the unum containing the POLE */
    uemp(&res);
    uint(&res, POLE, POLE);

    for (i = 0; i < sizeof(res.data); i++) {
        if (res.data[i]) {
            for (j = 0; j < sizeof(res.data[i]) * 8; j++) {
                if ((1 << (8 - j - 1)) & res.data[i]) {
                    break;
                }
            }
            pole = 8 * i + j;
            break;
        }
    }

    for (u = UCLAMP(pole, -NUMPOINTS); u <= UCLAMP(pole, +NUMPOINTS); u++) {
        /* fill res just with the single u */
        uemp(&res);
        res.data[u / 8] = 1 << (8 - (u % 8) - 1);
        uout(&res);
        printf("␣|->␣");

        /* calculate F(res) */
        uneg(&res);

        uemp(&tmp);
        uint(&tmp, 1, 1);
        uadd(&res, &tmp);

        uemp(&tmp);
        uint(&tmp, 3, 3);
        umul(&res, &tmp);

        uemp(&tmp);
        uint(&tmp, 1, 1);
        uadd(&res, &tmp);

        uabs(&res);
        ulog(&res);
        uout(&res);
        putchar('\n');
    }
    return 0;
}
