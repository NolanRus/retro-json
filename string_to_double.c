/*
 *  Source code for the "string_to_double" library procedure.
 *
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: strtod.c,v 1.1.1.4 2003/03/06 00:09:04 landonf Exp $
 */

#include <errno.h>
#include <ctype.h>
#include <stddef.h>

#include "string_to_double.h"

static int maxExponent = 511;   /* Largest possible base 10 exponent.  Any
                 * exponent larger than this will already
                 * produce underflow or overflow, so there's
                 * no need to worry about additional digits.
                 */

static double powersOf10[] = {  /* Table giving binary powers of 10.  Entry */
    10.,            /* is 10^2^i.  Used to convert decimal */
    100.,           /* exponents into floating-point numbers. */
    1.0e4,
    1.0e8,
    1.0e16,
    1.0e32,
    1.0e64,
    1.0e128,
    1.0e256
};

/*
 *----------------------------------------------------------------------
 *
 * string_to_double --
 *
 *  This procedure converts a floating-point number from an ASCII
 *  decimal representation to internal double-precision format.
 *
 * Results:
 *  The return value is the double-precision floating-point
 *  representation of the characters in string.  If endPtr isn't
 *  NULL, then *endPtr is filled in with the address of the
 *  next character after the last one that was part of the
 *  floating-point number.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

double string_to_double(const char * string, char ** endPtr) {
    int sign, expSign = 0;
    double fraction, dblExp, *d;
    register const char *p;
    register int c;
    int exp = 0;        /* Exponent read from "EX" field. */
    int fracExp = 0;        /* Exponent that derives from the fractional
                             * part.  Under normal circumstatnces, it is
                             * the negative of the number of digits in F.
                             * However, if I is very long, the last digits
                             * of I get dropped (otherwise a long I with a
                             * large negative exponent could cause an
                             * unnecessary overflow on I alone).  In this
                             * case, fracExp is incremented one for each
                             * dropped digit. */
    int mantSize;       /* Number of digits in mantissa. */
    int decPt;          /* Number of mantissa digits BEFORE decimal
                         * point. */
    const char *pExp;       /* Temporarily holds location of exponent
                             * in string. */
    /*
     * Strip off leading blanks and check for a sign.
     */
    p = string;
    while (isspace((unsigned char)*p)) {
    p += 1;
    }
    if (*p == '-') {
    sign = 1;
    p += 1;
    } else {
    if (*p == '+') {
        p += 1;
    }
    sign = 0;
    }
    /*
     * Count the number of digits in the mantissa (including the decimal
     * point), and also locate the decimal point.
     */

    decPt = -1;
    for (mantSize = 0; ; mantSize += 1)
    {
    c = *p;
    if (!isdigit(c)) {
        if ((c != '.') || (decPt >= 0)) {
        break;
        }
        decPt = mantSize;
    }
    p += 1;
    }

    /*
     * Now suck up the digits in the mantissa.  Use two integers to
     * collect 9 digits each (this is faster than using floating-point).
     * If the mantissa has more than 18 digits, ignore the extras, since
     * they can't affect the value anyway.
     */

    pExp  = p;
    p -= mantSize;
    if (decPt < 0) {
    decPt = mantSize;
    } else {
    mantSize -= 1;          /* One of the digits was the point. */
    }
    if (mantSize > 18) {
    fracExp = decPt - 18;
    mantSize = 18;
    } else {
    fracExp = decPt - mantSize;
    }
    if (mantSize == 0) {
    fraction = 0.0;
    p = string;
    goto done;
    } else {
    int frac1, frac2;
    frac1 = 0;
    for ( ; mantSize > 9; mantSize -= 1)
    {
        c = *p;
        p += 1;
        if (c == '.') {
        c = *p;
        p += 1;
        }
        frac1 = 10*frac1 + (c - '0');
    }
    frac2 = 0;
    for (; mantSize > 0; mantSize -= 1)
    {
        c = *p;
        p += 1;
        if (c == '.') {
        c = *p;
        p += 1;
        }
        frac2 = 10*frac2 + (c - '0');
    }
    fraction = (1.0e9 * frac1) + frac2;
    }

    /*
     * Skim off the exponent.
     */

    p = pExp;
    if ((*p == 'E') || (*p == 'e')) {
    p += 1;
    if (*p == '-') {
        expSign = 1;
        p += 1;
    } else {
        if (*p == '+') {
        p += 1;
        }
        expSign = 0;
    }
    if (!isdigit((unsigned char)*p)) {
        p = pExp;
        goto done;
    }
    while (isdigit((unsigned char)*p)) {
        exp = exp * 10 + (*p - '0');
        p += 1;
    }
    }
    if (expSign) {
    exp = fracExp - exp;
    } else {
    exp = fracExp + exp;
    }

    /*
     * Generate a floating-point number that represents the exponent.
     * Do this by processing the exponent one bit at a time to combine
     * many powers of 2 of 10. Then combine the exponent with the
     * fraction.
     */

    if (exp < 0) {
    expSign = 1;
    exp = -exp;
    } else {
    expSign = 0;
    }
    if (exp > maxExponent) {
    exp = maxExponent;
    errno = ERANGE;
    }
    dblExp = 1.0;
    for (d = powersOf10; exp != 0; exp >>= 1, d += 1) {
    if (exp & 01) {
        dblExp *= *d;
    }
    }
    if (expSign) {
    fraction /= dblExp;
    } else {
    fraction *= dblExp;
    }

done:
    if (endPtr != NULL) {
    *endPtr = (char *) p;
    }

    if (sign) {
    return -fraction;
    }
    return fraction;
}
