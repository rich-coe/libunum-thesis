#include <stdio.h>
int
main(void)
{
    double a, b, tmp;
    int i;

    a = 2;
    b = -4;

    for (i = 2; i < 26; i++) {
        tmp = 111 - 1130 / b + 3000 / (b * a);
        a = b;
        b = tmp;
        printf("u_%.2d␣=␣%f\n", i, b);
    }

    return 0;
}
