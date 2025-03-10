//---------------------------------------------------------------------------//
// Copyright (c) 2020-2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020-2021 Nikita Kaskov <nbering@nil.foundation>
// Copyright (c) 2020-2021 Pavel Kharitonov <ipavrus@nil.foundation>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_ALGEBRA_FIELDS_POWER_HPP
#define CRYPTO3_ALGEBRA_FIELDS_POWER_HPP

#include <nil/crypto3/multiprecision/integer.hpp>
#include <nil/crypto3/multiprecision/unsigned_utils.hpp>

namespace nil {
    namespace crypto3 {
        namespace algebra {
            namespace fields {
                namespace detail {
                    template<typename FieldValueType, typename NumberType>
                    constexpr FieldValueType power(const FieldValueType &base,
                                                   const NumberType &exponent_original) {
                        FieldValueType result = FieldValueType::one();

                        if (nil::crypto3::multiprecision::is_zero(exponent_original)) {
                            return result;
                        }

                        decltype(auto) exponent =
                            nil::crypto3::multiprecision::unsigned_or_throw(
                                exponent_original);

                        bool found_one = false;

                        for (long i = nil::crypto3::multiprecision::msb(exponent); i >= 0; --i) {
                            if (found_one) {
                                result = result.squared();
                            }

                            if (nil::crypto3::multiprecision::bit_test(exponent, i)) {
                                found_one = true;
                                result *= base;
                            }
                        }

                        return result;
                    }
                }    // namespace detail
            }        // namespace fields
        }            // namespace algebra
    }                // namespace crypto3
}    // namespace nil

#endif    // CRYPTO3_ALGEBRA_FIELDS_POWER_HPP
