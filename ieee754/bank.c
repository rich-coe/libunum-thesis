#include <float.h>
#include <math.h>
#include <stdio.h>

const int years = 25;

int
main(void)
{
    double a;
    int n;

    a = 1.718281828459045235;

    for (n = 1; n <= years; n++) {
        a = a * n - 1;
    }
    printf("u_%d␣=␣%f\n", years, a);
    return 0;
}
