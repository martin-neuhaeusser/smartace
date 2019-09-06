/**
 * Defines an interactive implementation of all non-deterministic primitive
 * value calls. Data is generated by prompting the user.
 * @date 2019
 */

#include "verify.h"

#include <stdio.h>

void on_entry(const char* _type, const char* _msg)
{
    printf("%s [%s]: ", _msg, _type);
}

void nd_mpz(mpz_t _dest, const char* _type, const char* _msg)
{
    on_entry(_type, _msg);
    mpz_inp_str(_dest, NULL, 10);
}

int8_t nd_int8_t(const char* _msg)
{
    on_entry("int8", _msg);
    int8_t retval;
    scanf("%hhd", &retval);
    return retval;
}

int16_t nd_int16_t(const char* _msg)
{
    on_entry("int16", _msg);
    int16_t retval;
    scanf("%hd", &retval);
    return retval;
}

int32_t nd_int32_t(const char* _msg)
{
    on_entry("int32", _msg);
    int32_t retval;
    scanf("%d", &retval);
    return retval;
}

int64_t nd_int64_t(const char* _msg)
{
    on_entry("int64", _msg);
    int64_t retval;
    scanf("%ld", &retval);
    return retval;
}

void nd_int128_t(mpz_t _dest, const char* _msg)
{
    nd_mpz(_dest, "int128", _msg);
}

void nd_int256_t(mpz_t _dest, const char* _msg)
{
    nd_mpz(_dest, "int256", _msg);
}

uint8_t nd_uint8_t(const char* _msg)
{
    on_entry("uint8", _msg);
    uint8_t retval;
    scanf("%hhd", &retval);
    return retval;
}

uint16_t nd_uint16_t(const char* _msg)
{
    on_entry("uint16", _msg);
    uint16_t retval;
    scanf("%hd", &retval);
    return retval;
}

uint32_t nd_uint32_t(const char* _msg)
{
    on_entry("uint32", _msg);
    uint32_t retval;
    scanf("%d", &retval);
    return retval;
}

uint64_t nd_uint64_t(const char* _msg)
{
    on_entry("uint64", _msg);
    uint64_t retval;
    scanf("%ld", &retval);
    return retval;
}

void nd_uint128_t(mpz_t _dest, const char* _msg)
{
    nd_mpz(_dest, "uint128", _msg);
}

void nd_uint256_t(mpz_t _dest, const char* _msg)
{
    nd_mpz(_dest, "uint256", _msg);
}
