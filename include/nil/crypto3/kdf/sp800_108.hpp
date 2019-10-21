//---------------------------------------------------------------------------//
// Copyright (c) 2019 Nil Foundation AG
// Copyright (c) 2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_KDF_SP800_108_HPP
#define CRYPTO3_KDF_SP800_108_HPP

#include <nil/crypto3/kdf/detail/sp800_108/sp800_108_functions.hpp>

namespace nil {
    namespace crypto3 {
        namespace kdf {
            namespace mode {
                /*!
                 * @brief sp800_108 key derivation function policy base class
                 * @tparam MessageAuthenticationCode
                 */
                template<typename MessageAuthenticationCode>
                struct sp800_108_mode {
                    typedef MessageAuthenticationCode mac_type;
                };
                /*!
                 * @brief NIST SP 800-108 KDF Counter Mode (5.1)
                 * @tparam MessageAuthenticationCode
                 */
                template<typename MessageAuthenticationCode>
                struct counter : sp800_108_mode<MessageAuthenticationCode> {
                    typedef typename sp800_108_mode<MessageAuthenticationCode>::mac_type mac_type;

                    inline static void process() {
                        const std::size_t prf_len = m_prf->output_length();
                        const uint8_t delim = 0;
                        const uint32_t length = static_cast<uint32_t>(key_len * 8);

                        uint8_t *p = key;
                        uint32_t counter = 1;
                        uint8_t be_len[4] = {0};
                        secure_vector<uint8_t> tmp;

                        store_be(length, be_len);
                        m_prf->set_key(secret, secret_len);

                        while (p < key + key_len && counter != 0) {
                            const std::size_t to_copy = std::min<std::size_t>(key + key_len - p, prf_len);
                            uint8_t be_cnt[4] = {0};

                            store_be(counter, be_cnt);

                            m_prf->update(be_cnt, 4);
                            m_prf->update(label, label_len);
                            m_prf->update(delim);
                            m_prf->update(salt, salt_len);
                            m_prf->update(be_len, 4);
                            m_prf->final(tmp);

                            copy_mem(p, tmp.data(), to_copy);
                            p += to_copy;

                            ++counter;
                            if (counter == 0) {
                                throw std::invalid_argument("Can't process more than 4GB");
                            }
                        }

                        return key_len;
                    }
                };

                /*!
                 * @brief  NIST SP 800-108 KDF Feedback Mode (5.2)
                 * @tparam MessageAuthenticationCode
                 */
                template<typename MessageAuthenticationCode>
                struct feedback : sp800_108_mode<MessageAuthenticationCode> {
                    typedef typename sp800_108_mode<MessageAuthenticationCode>::mac_type mac_type;

                    inline static void process() {
                        const uint32_t length = static_cast<uint32_t>(key_len * 8);
                        const std::size_t prf_len = m_prf->output_length();
                        const std::size_t iv_len = (salt_len >= prf_len ? prf_len : 0);
                        const uint8_t delim = 0;

                        uint8_t *p = key;
                        uint32_t counter = 1;
                        uint8_t be_len[4] = {0};
                        secure_vector<uint8_t> prev(salt, salt + iv_len);
                        secure_vector<uint8_t> ctx(salt + iv_len, salt + salt_len);

                        store_be(length, be_len);
                        m_prf->set_key(secret, secret_len);

                        while (p < key + key_len && counter != 0) {
                            const std::size_t to_copy = std::min<std::size_t>(key + key_len - p, prf_len);
                            uint8_t be_cnt[4] = {0};

                            store_be(counter, be_cnt);

                            m_prf->update(prev);
                            m_prf->update(be_cnt, 4);
                            m_prf->update(label, label_len);
                            m_prf->update(delim);
                            m_prf->update(ctx);
                            m_prf->update(be_len, 4);
                            m_prf->final(prev);

                            copy_mem(p, prev.data(), to_copy);
                            p += to_copy;

                            ++counter;

                            if (counter == 0) {
                                throw std::invalid_argument("Can't process more than 4GB");
                            }
                        }

                        return key_len;
                    }
                };

                /*!
                 * @brief NIST SP 800-108 KDF Double Pipeline Mode (5.3)
                 * @tparam MessageAuthenticationCode
                 */
                template<typename MessageAuthenticationCode>
                struct pipeline : sp800_108_mode<MessageAuthenticationCode> {
                    typedef typename sp800_108_mode<MessageAuthenticationCode>::mac_type mac_type;

                    inline static void process() {
                        const uint32_t length = static_cast<uint32_t>(key_len * 8);
                        const std::size_t prf_len = m_prf->output_length();
                        const uint8_t delim = 0;

                        uint8_t *p = key;
                        uint32_t counter = 1;
                        uint8_t be_len[4] = {0};
                        secure_vector<uint8_t> ai, ki;

                        store_be(length, be_len);
                        m_prf->set_key(secret, secret_len);

                        // A(0)
                        std::copy(label, label + label_len, std::back_inserter(ai));
                        ai.emplace_back(delim);
                        std::copy(salt, salt + salt_len, std::back_inserter(ai));
                        std::copy(be_len, be_len + 4, std::back_inserter(ai));

                        while (p < key + key_len && counter != 0) {
                            // A(i)
                            m_prf->update(ai);
                            m_prf->final(ai);

                            // K(i)
                            const std::size_t to_copy = std::min<std::size_t>(key + key_len - p, prf_len);
                            uint8_t be_cnt[4] = {0};

                            store_be(counter, be_cnt);

                            m_prf->update(ai);
                            m_prf->update(be_cnt, 4);
                            m_prf->update(label, label_len);
                            m_prf->update(delim);
                            m_prf->update(salt, salt_len);
                            m_prf->update(be_len, 4);
                            m_prf->final(ki);

                            copy_mem(p, ki.data(), to_copy);
                            p += to_copy;

                            ++counter;

                            if (counter == 0) {
                                throw std::invalid_argument("Can't process more than 4GB");
                            }
                        }

                        return key_len;
                    }
                };
            }    // namespace mode

            /*!
             * @brief NIST SP 800-108 KDF
             * @tparam Mode Mode intended to be used
             * @tparam MessageAuthenticationCode
             */
            template<typename MessageAuthenticationCode, template<typename> class Mode>
            class sp800_108 {
                typedef detail::sp800_108_functions<MessageAuthenticationCode, Mode> policy_type;

            public:
                typedef typename policy_type::mode_type mode_type;
                typedef typename policy_type::mac_type mac_type;

                static void process() {
                    mode_type::process();
                }
            };
        }    // namespace kdf
    }    // namespace crypto3
}    // namespace nil

#endif