/*
 * test-program.c - Simple test program for integration tests
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This program is used by integration tests to verify GDB functionality.
 * It provides simple functions that can be debugged.
 */

#include <stdio.h>

/*
 * add:
 * @a: first number
 * @b: second number
 *
 * Adds two integers together.
 *
 * Returns: the sum of a and b
 */
int
add (int a, int b)
{
    int result;

    result = a + b;
    return result;
}

/*
 * multiply:
 * @a: first number
 * @b: second number
 *
 * Multiplies two integers together.
 *
 * Returns: the product of a and b
 */
int
multiply (int a, int b)
{
    int result;

    result = a * b;
    return result;
}

/*
 * factorial:
 * @n: the number to compute factorial of
 *
 * Computes the factorial of n recursively.
 *
 * Returns: n!
 */
int
factorial (int n)
{
    if (n <= 1)
    {
        return 1;
    }
    return n * factorial (n - 1);
}

int
main (void)
{
    int x;
    int y;
    int sum;
    int product;
    int fact;

    x = 3;
    y = 4;

    sum = add (x, y);
    printf ("Sum: %d + %d = %d\n", x, y, sum);

    product = multiply (x, y);
    printf ("Product: %d * %d = %d\n", x, y, product);

    fact = factorial (5);
    printf ("Factorial: 5! = %d\n", fact);

    return 0;
}
