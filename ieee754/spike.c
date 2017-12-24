#include <float.h>
#include <math.h>
#include <stdio.h>

#define LEN(x) (sizeof (x) / sizeof *(x))
#define NUMPOINTS (10)
#define POLE (4.0/3.0)

int
main(void)
{
    double x[NUMPOINTS * 2 + 1][2];
    int i;

    /* fill POINT environment */
    x[NUMPOINTS][0] = POLE;
    for (i = NUMPOINTS - 1; i >= 0; i--) {
        x[i][0] = nextafter(x[i + 1][0], -INFINITY);
    }

    for (i = NUMPOINTS + 1; i < NUMPOINTS * 2 + 1; i++) {
        x[i][0] = nextafter(x[i - 1][0], +INFINITY);
    }

    /* calculate values of |3*(1-x)+1| in the POINT environment */
    for (i = 0; i < NUMPOINTS * 2 + 1; i++) {
        x[i][1] = fabs(3 * (1 - x[i][0]) + 1);
        printf("x-%.2f=%.20e␣|3*(1-x)+1|=%.20f\n", POLE, x[i][0]-POLE, x[i][1]);
    }

    putchar(’\n’);

    /* calculate values of f(x) in the POINT environment */
    for (i = 0; i < NUMPOINTS * 2 + 1; i++) {
        x[i][1] = log(fabs(3 * (1 - x[i][0]) + 1));
        printf("x-%.2f=%.20e␣f(x)=%.20f\n", POLE, x[i][0]-POLE, x[i][1]);
    }
    return 0;
}
