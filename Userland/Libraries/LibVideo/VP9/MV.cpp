/*
 * Copyright (c) 2021, Hunter Salyer <thefalsehonesty@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "MV.h"

namespace Video::VP9 {

MV::MV(i32 row, i32 col)
    : m_row(row)
    , m_col(col)
{
}

MV& MV::operator=(i32 value)
{
    m_row = value;
    m_col = value;
    return *this;
}

MV MV::operator+(MV const& other) const
{
    return MV(this->row() + other.row(), this->col() + other.col());
}

void MV::operator*=(i32 value)
{
    m_row = m_row * value;
    m_col = m_col * value;
}

bool MV::operator==(const MV& other) const
{
    return this->row() == other.row() && this->col() == other.col();
}

}
