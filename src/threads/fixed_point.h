#include <stdint.h>

#define INT_MAX ((1 << 31) - 1)
#define INT_MIN (-(1 << 31))

#define f (1 << 14) /* 17.14 format 1 */

int int_to_fp(int n);
int fp_to_int_round(int x);
int fp_to_int(int x);
int add_fp(int x, int y);
int add_mixed(int x, int n);
int sub_fp(int x, int y);
int sub_mixed(int x, int n);
int mul_fp(int x, int y);
int mul_mixed(int x, int y);
int div_fp(int x, int y);
int div_mixed(int x, int n);

int int_to_fp(int n)
{
    return n*f;
}

int fp_to_int_round(int x)
{
    return x/f;
}

int fp_to_int(int x)
{
    if(x>=0)
    {
        return (x+f/2)/f;
    }
    else
    {
        return (x-f/2)/f;
    }
}


int add_fp(int x, int y)
{
    return x+y;
}

int add_mixed(int x, int n)
{
    return x+n*f;
}

int sub_fp(int x, int y)
{
    return x-y;
}

int sub_mixed(int x, int n)
{
    return x-n*f;
}

int mul_fp(int x, int y)
{
    return ((int64_t) x)*y/f;
}

int mul_mixed(int x, int n)
{
    return x*n;
}

int div_fp(int x, int y)
{
    return ((int64_t) x)*f/y;
}

int div_mixed(int x, int n)
{
    return x/n;
}
