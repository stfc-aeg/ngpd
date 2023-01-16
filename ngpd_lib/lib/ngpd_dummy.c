#include <stdio.h>

int ngpd_dummy(int val)
{
    printf("Hello from ngpd_dummy with value %d\n", val);
    return val + 1;
}