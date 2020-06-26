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

#include <cmath>
#include "Complex.h"

Complex Complex::i = Complex(0, 1);
Complex Complex::j = Complex(0, 1);
Complex Complex::zero = Complex(0, 0);

Complex::Complex(double real, double imag)
    : m_real(real)
    , m_imag(imag)
{
}

Complex::Complex(const Complex& other)
    : m_real(other.real())
    , m_imag(other.imag())
{
}

Complex::~Complex()
{
}

double Complex::mag() const
{
    return sqrt(pow(m_real, 2) + pow(m_imag, 2));
}

double Complex::angle() const
{
    return atan2(m_imag, m_real);
}

Complex Complex::operator+(const Complex& other) const
{
    return { real() + other.real(), imag() + other.imag() };
}

Complex Complex::operator-(const Complex& other) const
{
    return { real() - other.real(), imag() - other.imag() };
}

Complex Complex::operator*(const Complex& other) const
{
    return {
        real() * other.real() - imag() * other.imag(),
        real() * other.imag() + imag() * other.real()
    };
}

Complex Complex::operator/(const Complex& other) const
{
    double divisor = pow(other.real(), 2.0) + pow(other.imag(), 2.0);
    double re = real() * other.real() + imag() * other.imag();
    double im = imag() * other.real() - real() * other.imag();

    return {
        re / divisor,
        im / divisor
    };
}

Complex& Complex::operator+=(const Complex& other)
{
    Complex temp = *this + other;
    m_real = temp.real();
    m_imag = temp.imag();
    return *this;
}

Complex& Complex::operator-=(const Complex& other)
{
    Complex temp = *this - other;
    m_real = temp.real();
    m_imag = temp.imag();
    return *this;
}

Complex& Complex::operator*=(const Complex& other)
{
    Complex temp = *this * other;
    m_real = temp.real();
    m_imag = temp.imag();
    return *this;
}

Complex& Complex::operator/=(const Complex& other)
{
    Complex temp = *this / other;
    m_real = temp.real();
    m_imag = temp.imag();
    return *this;
}

bool Complex::operator==(const Complex& other) const
{
    return real() == other.real() && imag() == other.imag();
}

Complex operator+(double real, const Complex& complex)
{
    return complex + real;
}

Complex operator-(double real, const Complex& complex)
{
    return complex - real;
}

Complex operator*(double real, const Complex& complex)
{
    return complex * real;
}

Complex operator/(double real, const Complex& complex)
{
    return Complex(real) / complex;
}


const LogStream& operator<<(const LogStream& stream, const Complex& complex)
{
    if (complex.real() == 0 && complex.imag() == 0) {
        stream << "0";
        return stream;
    }

    if (complex.real() != 0)
        stream << complex.real();

    if (complex.imag() < 0) {
        stream << "-j" << -complex.imag();
    } else if (complex.imag() > 0) {
        stream << "+j" << complex.imag();
    }

    return stream;
}

Complex iexp(double n)
{
    return { cos(n), sin(n) };
}

Complex exp(Complex n)
{
    return iexp(n.imag()) * exp(n.real());
}

Complex ilog(Complex n)
{
    return { log(n.mag()), n.angle() };
}
