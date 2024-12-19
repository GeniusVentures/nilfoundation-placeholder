//---------------------------------------------------------------------------//
// Copyright (c) 2019-2021 Aleksei Moskvin <alalmoskvin@nil.foundation>
// Copyright (c) 2020 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020 Ilias Khairullin <ilias@nil.foundation>
// Copyright (c) 2024 Andrey Nefedov <ioxid@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#pragma once

// IWYU pragma: private; include "nil/crypto3/multiprecision/big_uint.hpp"

#include <cstddef>
#include <type_traits>

#include "nil/crypto3/multiprecision/detail/big_mod/big_mod_impl.hpp"
#include "nil/crypto3/multiprecision/detail/big_mod/ops/pow.hpp"
#include "nil/crypto3/multiprecision/detail/big_uint/big_uint_impl.hpp"

namespace nil::crypto3::multiprecision {
    template<typename T1, std::size_t Bits, typename T2,
             std::enable_if_t<detail::is_integral_v<std::decay_t<T1>> &&
                                  detail::is_integral_v<std::decay_t<T2>>,
                              int> = 0>
    constexpr big_uint<Bits> powm(T1 &&b, T2 &&e, const big_uint<Bits> &m) {
        return pow(big_mod_rt<Bits>(std::forward<T1>(b), m), std::forward<T2>(e)).base();
    }
}  // namespace nil::crypto3::multiprecision
