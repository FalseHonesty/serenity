/*
 * Copyright (c) 2020, Matthew Olsson <matthewcolsson@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <AK/LogStream.h>

class Complex {
public:
    static Complex i;
    static Complex j;
    static Complex zero;

    Complex(double real = 0, double imag = 0);
    Complex(const Complex& other);
    ~Complex();

    ALWAYS_INLINE double real() const { return m_real; }
    ALWAYS_INLINE double imag() const { return m_imag; }

    ALWAYS_INLINE double& real() { return m_real; }
    ALWAYS_INLINE double& imag() { return m_imag; }

    ALWAYS_INLINE double mag() const;
    ALWAYS_INLINE double angle() const;

    Complex operator+(const Complex& other) const;
    Complex operator-(const Complex& other) const;
    Complex operator*(const Complex& other) const;
    Complex operator/(const Complex& other) const;

    ALWAYS_INLINE Complex operator+() const { return *this; }
    ALWAYS_INLINE Complex operator-() const { return { -real(), -imag() }; }

    Complex& operator+=(const Complex& other);
    Complex& operator-=(const Complex& other);
    Complex& operator*=(const Complex& other);
    Complex& operator/=(const Complex& other);

    ALWAYS_INLINE bool operator==(const Complex& other) const;

private:
    double m_real;
    double m_imag;
};

ALWAYS_INLINE Complex operator+(double real, const Complex& complex);
ALWAYS_INLINE Complex operator-(double real, const Complex& complex);
ALWAYS_INLINE Complex operator*(double real, const Complex& complex);
ALWAYS_INLINE Complex operator/(double real, const Complex& complex);

const LogStream& operator<<(const LogStream& stream, const Complex& complex);

Complex iexp(double n);
Complex exp(Complex n);
Complex ilog(Complex n);
