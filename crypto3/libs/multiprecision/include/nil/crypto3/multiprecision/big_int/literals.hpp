///////////////////////////////////////////////////////////////
//  Copyright 2013 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <array>
#include <cstddef>

#include "nil/crypto3/multiprecision/big_int/big_uint.hpp"

namespace nil::crypto3::multiprecision::literals {
    template<char... C>
    constexpr auto operator"" _bigui() {
        constexpr std::size_t N = sizeof...(C);
        static_assert(N > 2, "hex literal should start with 0x");
        constexpr std::array<char, N> str{C...};
        constexpr auto result =
            nil::crypto3::multiprecision::detail::parse_int_hex<(N - 2) * 4>({str.data(), N});
        return result;
    }
}  // namespace nil::crypto3::multiprecision::literals

#define NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(Bits)                                        \
    namespace nil::crypto3::multiprecision::literals {                                  \
        template<char... C>                                                             \
        constexpr auto operator"" _bigui##Bits() {                                      \
            constexpr std::size_t N = sizeof...(C);                                     \
            constexpr std::array<char, N> str{C...};                                    \
            constexpr auto result =                                                     \
                nil::crypto3::multiprecision::detail::parse_int<Bits>({str.data(), N}); \
            return result;                                                              \
        }                                                                               \
    }

// This is a comprehensive list of all bitlengths we use in algebra.
// Custom ones can be defined using this macro in every place where they are used.
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(4)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(7)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(8)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(13)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(15)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(16)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(17)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(18)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(64)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(92)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(94)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(128)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(130)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(149)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(150)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(151)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(152)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(160)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(161)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(163)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(164)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(177)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(178)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(179)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(180)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(181)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(182)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(183)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(191)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(192)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(205)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(206)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(222)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(223)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(224)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(225)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(226)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(239)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(248)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(249)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(250)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(251)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(252)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(253)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(254)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(255)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(256)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(257)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(263)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(264)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(280)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(281)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(292)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(293)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(294)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(295)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(296)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(297)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(298)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(315)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(316)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(319)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(320)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(330)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(331)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(374)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(375)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(376)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(377)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(378)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(379)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(380)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(381)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(384)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(503)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(504)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(507)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(512)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(515)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(516)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(521)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(546)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(577)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(578)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(585)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(595)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(636)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(706)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(707)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(758)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(753)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(759)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(761)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(859)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(860)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(893)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(894)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(913)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(1024)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(1490)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(1536)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(2048)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(2790)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(3072)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(4096)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(4269)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(4314)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(6144)
NIL_CO3_MP_DEFINE_BIG_UINT_LITERAL(8192)

using namespace nil::crypto3::multiprecision::literals;
