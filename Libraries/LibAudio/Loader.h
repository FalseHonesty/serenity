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

#include <AK/String.h>
#include <LibAudio/Buffer.h>
#include <LibCore/File.h>

#define ENUMERATE_AUDIO_TYPES(A) \
    A(Wav, wav)

namespace Audio {

enum AudioType {
#define ENUMERATE_AUDIO_TYPE(type, extension) type,
    ENUMERATE_AUDIO_TYPES(ENUMERATE_AUDIO_TYPE)
#undef ENUMERATE_AUDIO_TYPE
};

#define ENUMERATE_AUDIO_TYPE(type, extension) class type##Loader;
    ENUMERATE_AUDIO_TYPES(ENUMERATE_AUDIO_TYPE)
#undef ENUMERATE_AUDIO_TYPE

class Loader {
public:
    static OwnPtr<Loader> load_from_file(const StringView& path);

    Loader(const StringView& path);
    virtual ~Loader() = default;

    virtual AudioType type() const = 0;
    virtual String type_name() const = 0;

    bool has_error() const { return !m_error.is_null(); }
    String error() const {  return m_error; };
    virtual bool sniff() = 0;

    void reset() { seek(0); };
    virtual void seek(int position) = 0;

    virtual int number_of_samples() const = 0;
    virtual int number_of_loaded_samples() const = 0;
    virtual u32 sample_rate() const = 0;
    virtual u16 number_of_channels() const = 0;
    virtual u16 bits_per_sample() const = 0;

    virtual RefPtr<Buffer> get_more_samples(size_t max_bytes_to_read_from_input = 128 * KB) = 0;
    RefPtr<Core::File> file() const { return m_file; };

protected:
    RefPtr<Core::File> m_file;
    String m_error;
};

}
