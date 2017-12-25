#include <stdio.h>
#include "unum.h"

int
main(void)
{
    SORN a, b, c, tmp1, tmp2, tmp3;
    int n;

    uemp(&a);
    uemp(&b);
    uemp(&c);

    uint(&a, 2, 2);
    uint(&b, -4, -4);

    for (n = 2; n <= 25; n++) {
        if (n > 2) {
            uset(&a, &b);
            uset(&b, &c);
        }
        uemp(&tmp1);
        uint(&tmp1, 111, 111);

        uemp(&tmp2);
        uint(&tmp2, 1130, 1130);
        udiv(&tmp2, &b);
        usub(&tmp1, &tmp2);

        uemp(&tmp2);
        uint(&tmp2, 3000, 3000);
        uset(&tmp3, &b);
        umul(&tmp3, &a);

        udiv(&tmp2, &tmp3);
        uadd(&tmp1, &tmp2);

        uset(&c, &tmp1);

        printf("U_%d = ", n);
        uout(&c);
        putchar('\n');
    }
    return 0;
}
