/*!
 * \file glonass_l1_signal_processing.cc
 * \brief This class implements various functions for GLONASS L1 CA signals
 * \author Javier Arribas, 2011. jarribas(at)cttc.es
 *
 *
 * -----------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2020  (see AUTHORS file for a list of contributors)
 *
 * GNSS-SDR is a software defined Global Navigation
 *          Satellite Systems receiver
 *
 * This file is part of GNSS-SDR.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * -----------------------------------------------------------------------------
 */

#include "glonass_l1_signal_processing.h"
#include <array>
#include <bitset>

const auto AUX_CEIL = [](float x) { return static_cast<int32_t>(static_cast<int64_t>((x) + 1)); };

void glonass_l1_ca_code_gen_complex(own::span<std::complex<float>> _dest, uint32_t _chip_shift)
{
    const uint32_t _code_length = 511;
    std::bitset<_code_length> G1{};
    auto G1_register = std::bitset<9>{}.set();  // All true
    bool feedback1;
    bool aux;
    uint32_t delay;
    uint32_t lcv;
    uint32_t lcv2;

    /* Generate G1 Register */
    for (lcv = 0; lcv < _code_length; lcv++)
        {
            G1[lcv] = G1_register[2];

            feedback1 = G1_register[4] xor G1_register[0];

            for (lcv2 = 0; lcv2 < 8; lcv2++)
                {
                    G1_register[lcv2] = G1_register[lcv2 + 1];
                }

            G1_register[8] = feedback1;
        }

    /* Generate PRN from G1 Register */
    for (lcv = 0; lcv < _code_length; lcv++)
        {
            aux = G1[lcv];
            if (aux == true)
                {
                    _dest[lcv] = std::complex<float>(1, 0);
                }
            else
                {
                    _dest[lcv] = std::complex<float>(-1, 0);
                }
        }

    /* Set the delay */
    delay = _code_length;
    delay += _chip_shift;
    delay %= _code_length;

    /* Generate PRN from G1 and G2 Registers */
    for (lcv = 0; lcv < _code_length; lcv++)
        {
            aux = G1[(lcv + _chip_shift) % _code_length];
            if (aux == true)
                {
                    _dest[lcv] = std::complex<float>(1, 0);
                }
            else
                {
                    _dest[lcv] = std::complex<float>(-1, 0);
                }
            delay++;
            delay %= _code_length;
        }
}


/*
 *  Generates complex GLONASS L1 C/A code for the desired SV ID and sampled to specific sampling frequency
 */
void glonass_l1_ca_code_gen_complex_sampled(own::span<std::complex<float>> _dest, int32_t _fs, uint32_t _chip_shift)
{
    constexpr int32_t _codeFreqBasis = 511000;  // Hz
    constexpr int32_t _codeLength = 511;
    constexpr float _tc = 1.0 / static_cast<float>(_codeFreqBasis);  // C/A chip period in sec

    const float _ts = 1.0F / static_cast<float>(_fs);  // Sampling period in sec
    const auto _samplesPerCode = static_cast<int32_t>(static_cast<double>(_fs) / (static_cast<double>(_codeFreqBasis) / static_cast<double>(_codeLength)));

    std::array<std::complex<float>, 511> _code{};
    int32_t _codeValueIndex;
    float aux;

    glonass_l1_ca_code_gen_complex(_code, _chip_shift);  // generate C/A code 1 sample per chip

    for (int32_t i = 0; i < _samplesPerCode; i++)
        {
            // === Digitizing ==================================================

            // --- Make index array to read C/A code values --------------------
            // The length of the index array depends on the sampling frequency -
            // number of samples per millisecond (because one C/A code period is one
            // millisecond).

            aux = (_ts * (static_cast<float>(i) + 1)) / _tc;
            _codeValueIndex = AUX_CEIL(aux) - 1;

            // --- Make the digitized version of the C/A code ------------------
            // The "upsampled" code is made by selecting values form the CA code
            // chip array (caCode) for the time instances of each sample.
            if (i == _samplesPerCode - 1)
                {
                    // --- Correct the last index (due to number rounding issues) -----------
                    _dest[i] = _code[_codeLength - 1];
                }
            else
                {
                    _dest[i] = _code[_codeValueIndex];  // repeat the chip -> upsample
                }
        }
}
