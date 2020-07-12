/*
 * Copyright (c) 2020, Hunter Salyer <thefalsehonesty@gmail.com>
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

#include "VP9Decoder.h"

namespace Video {

#define RESERVED_ZERO                  \
    if (m_bit_stream->read_bit() != 0) return false

bool VP9Decoder::parse_frame(const ByteBuffer& frame_data)
{
    m_bit_stream = make<BitStream>(frame_data.data(), frame_data.size());
    m_probability_tables = make<ProbabilityTables>();
    m_probability_tables = make<SyntaxElementCounter>();

    m_start_bit_pos = m_bit_stream->get_position();
    if (!uncompressed_header())
        return false;
    if (!trailing_bits())
        return false;
    if (m_header_size_in_bytes == 0) {
        // TODO: Do we really need to read all of these bits?
        // while (m_bit_stream->get_position() < m_start_bit_pos + (8 * frame_data.size()))
        //     RESERVED_ZERO;
        return true;
    }
    m_probability_tables->load_probs(m_frame_context_idx);
    m_probability_tables->load_probs2(m_frame_context_idx);
    m_syntax_element_counter->clear_counts();
    m_bit_stream->init_bool(m_header_size_in_bytes);
    // TODO: compressed_header();
    m_bit_stream->exit_bool();
    return true;
}

bool VP9Decoder::uncompressed_header()
{
    auto frame_marker = m_bit_stream->read_f(2);
    if (frame_marker != 2)
        return false;
    auto profile_low_bit = m_bit_stream->read_bit();
    auto profile_high_bit = m_bit_stream->read_bit();
    m_profile = (profile_high_bit << 1u) + profile_low_bit;
    if (m_profile == 3)
        RESERVED_ZERO;
    auto show_existing_frame = m_bit_stream->read_bit();
    if (show_existing_frame) {
        m_frame_to_show_map_index = m_bit_stream->read_f(3);
        m_header_size_in_bytes = 0;
        m_refresh_frame_flags = 0;
        m_loop_filter_level = 0;
        return true;
    }

    m_last_frame_type = m_frame_type;
    m_frame_type = read_frame_type();
    m_show_frame = m_bit_stream->read_bit();
    m_error_resilient_mode = m_bit_stream->read_bit();

    if (m_frame_type == KEY_FRAME) {
        if (!frame_sync_code())
            return false;
        if (!color_config())
            return false;
        if (!frame_size())
            return false;
        if (!render_size())
            return false;
        m_refresh_frame_flags = 0xFF;
        m_frame_is_intra = true;
    } else {
        m_frame_is_intra = !m_show_frame && m_bit_stream->read_bit();

        if (!m_error_resilient_mode) {
            m_reset_frame_context = m_bit_stream->read_f(2);
        } else {
            m_reset_frame_context = 0;
        }

        if (m_frame_is_intra) {
            if (!frame_sync_code())
                return false;
            if (m_profile > 0) {
                if (!color_config())
                    return false;
            } else {
                m_color_space = CS_BT_601;
                m_subsampling_x = true;
                m_subsampling_y = true;
                m_bit_depth = 8;
            }

            m_refresh_frame_flags = m_bit_stream->read_f8();
            if (!frame_size())
                return false;
            if (!render_size())
                return false;
        } else {
            m_refresh_frame_flags = m_bit_stream->read_f8();
            for (auto i = 0; i < 3; i++) {
                m_ref_frame_idx[i] = m_bit_stream->read_f(3);
                m_ref_frame_sign_bias[LAST_FRAME + i] = m_bit_stream->read_bit();
            }
            frame_size_with_refs();
            m_allow_high_precision_mv = m_bit_stream->read_bit();
            read_interpolation_filter();
        }
    }

    if (!m_error_resilient_mode) {
        m_refresh_frame_context = m_bit_stream->read_bit();
        m_frame_parallel_decoding_mode = m_bit_stream->read_bit();
    } else {
        m_refresh_frame_context = false;
        m_frame_parallel_decoding_mode = true;
    }

    m_frame_context_idx = m_bit_stream->read_f(2);
    if (m_frame_is_intra || m_error_resilient_mode) {
        setup_past_independence();
        if (m_frame_type == KEY_FRAME || m_error_resilient_mode || m_reset_frame_context == 3) {
            for (auto i = 0; i < 4; i++) {
                m_probability_tables->save_probs(i);
            }
        } else if (m_reset_frame_context == 2) {
            m_probability_tables->save_probs(m_frame_context_idx);
        }
        m_frame_context_idx = 0;
    }

    loop_filter_params();
    quantization_params();
    segmentation_params();
    tile_info();

    m_header_size_in_bytes = m_bit_stream->read_f16();

    return true;
}

bool VP9Decoder::frame_sync_code()
{
    if (m_bit_stream->read_byte() != 0x49)
        return false;
    if (m_bit_stream->read_byte() != 0x83)
        return false;
    return m_bit_stream->read_byte() == 0x42;
}

bool VP9Decoder::color_config()
{
    if (m_profile >= 2) {
        m_bit_depth = m_bit_stream->read_bit() ? 12 : 10;
    } else {
        m_bit_depth = 8;
    }

    auto color_space = m_bit_stream->read_f(3);
    if (color_space > CS_RGB)
        return false;
    m_color_space = static_cast<ColorSpace>(color_space);

    if (color_space != CS_RGB) {
        m_color_range = read_color_range();
        if (m_profile == 1 || m_profile == 3) {
            m_subsampling_x = m_bit_stream->read_bit();
            m_subsampling_y = m_bit_stream->read_bit();
            RESERVED_ZERO;
        } else {
            m_subsampling_x = true;
            m_subsampling_y = true;
        }
    } else {
        m_color_range = FULL_SWING;
        if (m_profile == 1 || m_profile == 3) {
            m_subsampling_x = false;
            m_subsampling_y = false;
            RESERVED_ZERO;
        }
    }
    return true;
}

bool VP9Decoder::frame_size()
{
    m_frame_width = m_bit_stream->read_f16() + 1;
    m_frame_height = m_bit_stream->read_f16() + 1;
    compute_image_size();
    return true;
}

bool VP9Decoder::render_size()
{
    if (m_bit_stream->read_bit()) {
        m_render_width = m_bit_stream->read_f16() + 1;
        m_render_height = m_bit_stream->read_f16() + 1;
    } else {
        m_render_width = m_frame_width;
        m_render_height = m_frame_height;
    }
    return true;
}

bool VP9Decoder::frame_size_with_refs()
{
    bool found_ref;
    for (auto i = 0; i < 3; i++) {
        found_ref = m_bit_stream->read_bit();
        if (found_ref) {
            // TODO:
            //  - FrameWidth = RefFrameWidth[ref_frame_idx[ i] ];
            //  - FrameHeight = RefFrameHeight[ref_frame_idx[ i] ];
            break;
        }
    }

    if (!found_ref)
        frame_size();
    else
        compute_image_size();

    render_size();
    return true;
}

bool VP9Decoder::compute_image_size()
{
    m_mi_cols = (m_frame_width + 7u) >> 3u;
    m_mi_rows = (m_frame_height + 7u) >> 3u;
    m_sb64_cols = (m_mi_cols + 7u) >> 3u;
    m_sb64_rows = (m_mi_rows + 7u) >> 3u;
    return true;
}

bool VP9Decoder::read_interpolation_filter()
{
    if (m_bit_stream->read_bit()) {
        m_interpolation_filter = SWITCHABLE;
    } else {
        m_interpolation_filter = literal_to_type[m_bit_stream->read_f(2)];
    }
    return true;
}

bool VP9Decoder::loop_filter_params()
{
    m_loop_filter_level = m_bit_stream->read_f(6);
    m_loop_filter_sharpness = m_bit_stream->read_f(3);
    m_loop_filter_delta_enabled = m_bit_stream->read_bit();
    if (m_loop_filter_delta_enabled) {
        if (m_bit_stream->read_bit()) {
            for (auto i = 0; i < 4; i++) {
                if (m_bit_stream->read_bit()) {
                    // TODO: loop_filter_ref_deltas[i] = s(6);
                }
            }
            for (auto i = 0; i < 2; i++) {
                if (m_bit_stream->read_bit()) {
                    // TODO: loop_filter_mode_deltas[i] = s(6);
                }
            }
        }
    }
    return true;
}

bool VP9Decoder::quantization_params()
{
    auto base_q_idx = m_bit_stream->read_byte();
    auto delta_q_y_dc = read_delta_q();
    auto delta_q_uv_dc = read_delta_q();
    auto delta_q_uv_ac = read_delta_q();
    m_lossless = base_q_idx == 0 && delta_q_y_dc == 0 && delta_q_uv_dc == 0 && delta_q_uv_ac == 0;
    return true;
}

i8 VP9Decoder::read_delta_q()
{
    if (m_bit_stream->read_bit())
        return m_bit_stream->read_s(4);
    return 0;
}

static constexpr u8 segmentation_feature_bits[SEG_LVL_MAX] = { 8, 6, 2, 0 };
static constexpr bool segmentation_feature_signed[SEG_LVL_MAX] = { true, true, false, false };

bool VP9Decoder::segmentation_params()
{
    auto segmentation_enabled = m_bit_stream->read_bit();
    if (!segmentation_enabled)
        return true;

    auto segmentation_update_map = m_bit_stream->read_bit();
    if (segmentation_update_map) {
        for (auto i = 0; i < 7; i++) {
            m_segmentation_tree_probs[i] = read_prob();
        }
        auto segmentation_temporal_update = m_bit_stream->read_bit();
        for (auto i = 0; i < 3; i++) {
            m_segmentation_pred_prob[i] = segmentation_temporal_update ? read_prob() : 255;
        }
    }

    if (!m_bit_stream->read_bit())
        return true;

    m_segmentation_abs_or_delta_update = m_bit_stream->read_bit();
    for (auto i = 0; i < MAX_SEGMENTS; i++) {
        for (auto j = 0; j < SEG_LVL_MAX; j++) {
            auto feature_value = 0;
            auto feature_enabled = m_bit_stream->read_bit();
            m_feature_enabled[i][j] = feature_enabled;
            if (feature_enabled) {
                auto bits_to_read = segmentation_feature_bits[j];
                feature_value = m_bit_stream->read_f(bits_to_read);
                if (segmentation_feature_signed[j]) {
                    if (m_bit_stream->read_bit())
                        feature_value = -feature_value;
                }
            }
            m_feature_data[i][j] = feature_value;
        }
    }
    return true;
}

u8 VP9Decoder::read_prob()
{
    if (m_bit_stream->read_bit())
        return m_bit_stream->read_byte();
    return 255;
}

bool VP9Decoder::tile_info()
{
    auto min_log2_tile_cols = calc_min_log2_tile_cols();
    auto max_log2_tile_cols = calc_max_log2_tile_cols();
    m_tile_cols_log2 = min_log2_tile_cols;
    while (m_tile_cols_log2 < max_log2_tile_cols) {
        if (m_bit_stream->read_bit())
            m_tile_cols_log2++;
        else
            break;
    }
    m_tile_rows_log2 = m_bit_stream->read_bit();
    if (m_tile_rows_log2) {
        m_tile_rows_log2 += m_bit_stream->read_bit();
    }
    return true;
}

u16 VP9Decoder::calc_min_log2_tile_cols()
{
    auto min_log_2 = 0;
    while ((MAX_TILE_WIDTH_B64 << min_log_2) < m_sb64_cols)
        min_log_2++;
    return min_log_2;
}

u16 VP9Decoder::calc_max_log2_tile_cols()
{
    auto max_log_2 = 1;
    while ((m_sb64_cols >> max_log_2) >= MIN_TILE_WIDTH_B64)
        max_log_2++;
    return max_log_2 - 1;
}

bool VP9Decoder::setup_past_independence()
{
    for (auto i = 0; i < 8; i++) {
        for (auto j = 0; j < 4; j++) {
            m_feature_data[i][j] = 0;
            m_feature_enabled[i][j] = false;
        }
    }
    m_segmentation_abs_or_delta_update = false;
    for (auto row = 0; row < m_mi_rows; row++) {
        for (auto col = 0; col < m_mi_cols; col++) {
            // TODO: m_prev_segment_ids[row][col] = 0;
        }
    }
    m_loop_filter_delta_enabled = true;
    m_loop_filter_ref_deltas[INTRA_FRAME] = 1;
    m_loop_filter_ref_deltas[LAST_FRAME] = 0;
    m_loop_filter_ref_deltas[GOLDEN_FRAME] = -1;
    m_loop_filter_ref_deltas[ALTREF_FRAME] = -1;
    for (auto i = 0; i < 2; i++) {
        m_loop_filter_mode_deltas[i] = 0;
    }
    m_probability_tables->reset_probs();
    return true;
}

bool VP9Decoder::trailing_bits()
{
    while (m_bit_stream->get_position() & 7u)
        RESERVED_ZERO;
    return true;
}

void VP9Decoder::dump_info()
{
    dbg() << "Frame dimensions: " << m_frame_width << "x" << m_frame_height;
    dbg() << "Render dimensions: " << m_render_width << "x" << m_render_height;
    dbg() << "Bit depth: " << m_bit_depth;
    dbg() << "Interpolation filter: " << m_interpolation_filter;
}

}
