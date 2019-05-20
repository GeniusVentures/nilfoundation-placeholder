//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nilfoundation.org>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_HASH_DETAIL_SHA2_POLICY_HPP
#define CRYPTO3_HASH_DETAIL_SHA2_POLICY_HPP

#include <nil/crypto3/block/detail/shacal/shacal2_policy.hpp>

#include <nil/crypto3/hash/detail/static_digest.hpp>

#include <boost/cstdint.hpp>
#include <array>

namespace nil {
    namespace crypto3 {
        namespace hash {
            namespace detail {

                template<unsigned CipherVersion>
                struct basic_sha2_policy {

                    constexpr static const std::size_t cipher_version = CipherVersion;

                    typedef block::detail::shacal2_policy<cipher_version> cipher_policy;
                    typedef typename cipher_policy::block_type state_type;

                };

                template<std::size_t Version>
                struct sha2_policy;

                template<>
                struct sha2_policy<224> : basic_sha2_policy<256> {

                    constexpr static const std::size_t digest_bits = 224;
                    constexpr static const std::uint8_t ieee1363_hash_id = 0x38;

                    typedef hash::static_digest<digest_bits> digest_type;
                    typedef std::array<std::uint8_t, 19> pkcs_id_type;

                    constexpr static const pkcs_id_type pkcs_id = {
                            0x30, 0x2D, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x04,
                            0x05, 0x00, 0x04, 0x1C
                    };

                    struct iv_generator {
                        state_type const &operator()() const {
                            static state_type const H0 = {{0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939, 0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4}};
                            return H0;
                        }
                    };

                };

                template<>
                struct sha2_policy<256> : basic_sha2_policy<256> {

                    constexpr static const std::size_t digest_bits = 256;
                    constexpr static const std::uint8_t ieee1363_hash_id = 0x34;

                    typedef hash::static_digest<digest_bits> digest_type;
                    typedef std::array<std::uint8_t, 19> pkcs_id_type;

                    constexpr static const pkcs_id_type pkcs_id = {
                            0x30, 0x2D, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x04,
                            0x05, 0x00, 0x04, 0x20
                    };

                    struct iv_generator {
                        state_type const &operator()() const {
                            static state_type const H0 = {{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19}};
                            return H0;
                        }
                    };

                };

                template<>
                struct sha2_policy<384> : basic_sha2_policy<512> {

                    constexpr static const std::size_t digest_bits = 384;
                    constexpr static const std::uint8_t ieee1363_hash_id = 0x36;

                    typedef hash::static_digest<digest_bits> digest_type;
                    typedef std::array<std::uint8_t, 19> pkcs_id_type;

                    constexpr static const pkcs_id_type pkcs_id = {
                            0x30, 0x2D, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x04,
                            0x05, 0x00, 0x04, 0x30
                    };

                    struct iv_generator {
                        state_type const &operator()() const {
                            static state_type const H0 = {{
                                                                  UINT64_C(0xcbbb9d5dc1059ed8), UINT64_C(
                                                                                                        0x629a292a367cd507), UINT64_C(
                                                                                                                                     0x9159015a3070dd17), UINT64_C(
                                                                                                                                                                  0x152fecd8f70e5939), UINT64_C(
                                                                                                                                                                                               0x67332667ffc00b31), UINT64_C(
                                                                                                                                                                                                                            0x8eb44a8768581511), UINT64_C(
                                                                                                                                                                                                                                                         0xdb0c2e0d64f98fa7), UINT64_C(
                                                                                                                                                                                                                                                                                      0x47b5481dbefa4fa4)
                                                          }};
                            return H0;
                        }
                    };

                };

                template<>
                struct sha2_policy<512> : basic_sha2_policy<512> {

                    constexpr static const std::size_t digest_bits = 512;
                    constexpr static const std::uint8_t ieee1363_hash_id = 0x35;

                    typedef hash::static_digest<digest_bits> digest_type;
                    typedef std::array<std::uint8_t, 19> pkcs_id_type;

                    constexpr static const pkcs_id_type pkcs_id = {
                            0x30, 0x2D, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x04,
                            0x05, 0x00, 0x04, 0x40
                    };

                    struct iv_generator {
                        state_type const &operator()() const {
                            static state_type const H0 = {{
                                                                  UINT64_C(0x6a09e667f3bcc908), UINT64_C(
                                                                                                        0xbb67ae8584caa73b), UINT64_C(
                                                                                                                                     0x3c6ef372fe94f82b), UINT64_C(
                                                                                                                                                                  0xa54ff53a5f1d36f1), UINT64_C(
                                                                                                                                                                                               0x510e527fade682d1), UINT64_C(
                                                                                                                                                                                                                            0x9b05688c2b3e6c1f), UINT64_C(
                                                                                                                                                                                                                                                         0x1f83d9abfb41bd6b), UINT64_C(
                                                                                                                                                                                                                                                                                      0x5be0cd19137e2179)
                                                          }};
                            return H0;
                        }
                    };

                };

            } // namespace detail
        }
    }
} // namespace nil

#endif // CRYPTO3_HASH_DETAIL_SHA2_POLICY_HPP
