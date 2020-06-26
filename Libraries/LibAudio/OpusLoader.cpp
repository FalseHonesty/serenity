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

#include <AK/ByteBuffer.h>
#include <LibAudio/OpusLoader.h>

namespace Audio {

#define PACKET_SECOND_BYTE_INDICATOR_MIN_VALUE 252

enum class Mode {
    SilkOnly,
    Hybrid,
    CeltOnly,
};

enum class Bandwidth {
    NarrowBand,
    MediumBand,
    WideBand,
    SuperWideBand,
    Fullband,
};

enum class FrameCountCode {
    OneFrame = 0,
    TwoFramesEqualSize,
    TwoFramesUnequalSize,
    ManyFrames,
};

struct TableOfContents {
    Mode mode;
    Bandwidth bandwidth;
    float frame_size_in_ms;
    bool is_stereo;

    /*
     * 0: 1 frame in packet
     * 1: 2 frames in packet with equal compressed sizes
     * 2: 2 frames in packet with unequal compressed sizes
     * 3: arbitrary number of frames in the packet;
     */
    FrameCountCode frame_count_code;
};

const Mode mode_lookup_table[32] = {
    Mode::SilkOnly, Mode::SilkOnly, Mode::SilkOnly, Mode::SilkOnly,
    Mode::SilkOnly, Mode::SilkOnly, Mode::SilkOnly, Mode::SilkOnly,
    Mode::SilkOnly, Mode::SilkOnly, Mode::SilkOnly, Mode::SilkOnly,
    Mode::Hybrid, Mode::Hybrid, Mode::Hybrid, Mode::Hybrid,
    Mode::CeltOnly, Mode::CeltOnly, Mode::CeltOnly, Mode::CeltOnly,
    Mode::CeltOnly, Mode::CeltOnly, Mode::CeltOnly, Mode::CeltOnly,
    Mode::CeltOnly, Mode::CeltOnly, Mode::CeltOnly, Mode::CeltOnly,
    Mode::CeltOnly, Mode::CeltOnly, Mode::CeltOnly, Mode::CeltOnly,
};

const Bandwidth bandwidth_lookup_table[32] = {
    Bandwidth::NarrowBand, Bandwidth::NarrowBand, Bandwidth::NarrowBand, Bandwidth::NarrowBand,
    Bandwidth::MediumBand, Bandwidth::MediumBand, Bandwidth::MediumBand, Bandwidth::MediumBand,
    Bandwidth::WideBand, Bandwidth::WideBand, Bandwidth::WideBand, Bandwidth::WideBand,
    Bandwidth::SuperWideBand, Bandwidth::SuperWideBand,
    Bandwidth::Fullband, Bandwidth::Fullband,
    Bandwidth::NarrowBand, Bandwidth::NarrowBand, Bandwidth::NarrowBand, Bandwidth::NarrowBand,
    Bandwidth::MediumBand, Bandwidth::MediumBand, Bandwidth::MediumBand, Bandwidth::MediumBand,
    Bandwidth::WideBand, Bandwidth::WideBand, Bandwidth::WideBand, Bandwidth::WideBand,
    Bandwidth::Fullband, Bandwidth::Fullband, Bandwidth::Fullband, Bandwidth::Fullband,
};

const float frame_size_in_ms_lookup_table[32] = {
    10, 20, 40, 60,
    10, 20, 40, 60,
    10, 20, 40, 60,
    10, 20,
    10, 20,
    2.5, 5, 10, 20,
    2.5, 5, 10, 20,
    2.5, 5, 10, 20,
    2.5, 5, 10, 20,
};

OpusLoader::OpusLoader()
{
}

TableOfContents OpusLoader::parse_table_of_contents(const ByteBuffer& buffer)
{
    u8 toc_byte = buffer[0];
    u8 config = toc_byte >> 3;
    bool stereo_bit = toc_byte & 0x4;
    u8 frame_count_code = toc_byte & 0x3;

    ASSERT(frame_count_code <= 3);

    return TableOfContents {
        mode_lookup_table[config],
        bandwidth_lookup_table[config],
        frame_size_in_ms_lookup_table[config],
        stereo_bit,
        static_cast<FrameCountCode>(frame_count_code),
    };
}

RefPtr<Buffer> OpusLoader::parse_frame(const ByteBuffer& buffer)
{
    if (!buffer.size()) {
        // Packet violates [R1]
        return {};
    }

    auto toc = parse_table_of_contents(buffer);

    if (toc.frame_count_code == FrameCountCode::OneFrame)
        return parse_single_frame(toc, buffer);

    ASSERT_NOT_REACHED();
}

RefPtr<Buffer> OpusLoader::parse_single_frame(const TableOfContents&, const ByteBuffer&)
{
    return {};
}

}
