/*
 * Copyright (c) 2021, Hunter Salyer <thefalsehonesty@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>

namespace Video::VP9 {

class MV {
public:
    MV() = default;
    MV(i32 row, i32 col);
    MV(MV const& other) = default;

    static MV Zero() { return MV(0, 0); }

    i32 row() const { return m_row; }
    void set_row(i32 row) { m_row = row; }
    i32 col() const { return m_col; }
    void set_col(i32 col) { m_col = col; }

    MV& operator=(i32 value);
    MV operator+(MV const& other) const;
    void operator*=(i32 value);
    bool operator==(MV const& other) const;

private:
    i32 m_row { 0 };
    i32 m_col { 0 };
};

}
