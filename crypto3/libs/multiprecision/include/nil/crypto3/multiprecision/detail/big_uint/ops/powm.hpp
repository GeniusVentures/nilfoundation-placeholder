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

#include <type_traits>

#include "nil/crypto3/multiprecision/detail/big_mod/big_mod_impl.hpp"
#include "nil/crypto3/multiprecision/detail/big_mod/ops/pow.hpp"
#include "nil/crypto3/multiprecision/detail/big_uint/big_uint_impl.hpp"

namespace nil::crypto3::multiprecision {
    template<typename T1, typename T2, typename T3,
             std::enable_if_t<detail::is_integral_v<std::decay_t<T1>> &&
                                  detail::is_integral_v<std::decay_t<T2>> &&
                                  detail::is_integral_v<std::decay_t<T3>>,
                              int> = 0>
    constexpr std::decay_t<T3> powm(T1 &&b, T2 &&e, T3 &&m) {
        using big_mod_t = big_mod_rt<
            std::decay_t<decltype(detail::as_big_uint(detail::unsigned_or_throw(m)))>::Bits>;
        return static_cast<std::decay_t<T3>>(
            pow(big_mod_t(std::forward<T1>(b), std::forward<T3>(m)), std::forward<T2>(e)).base());
    }
}  // namespace nil::crypto3::multiprecision
