//---------------------------------------------------------------------------//
// Copyright (c) 2017-2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020-2021 Nikita Kaskov <nbering@nil.foundation>
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

#ifndef MARSHALLING_STATIC_STRING_HPP
#define MARSHALLING_STATIC_STRING_HPP

#include <algorithm>
#include <iterator>
#include <string>
#include <initializer_list>

#include <nil/marshalling/assert_type.hpp>
#include <nil/marshalling/container/static_vector.hpp>

namespace nil::crypto3 {
    namespace marshalling {
        namespace container {
            namespace detail {

                template<typename TChar>
                class static_string_base {
                    using vector_type = static_vector_base<TChar>;
                    using cell_type = typename vector_type::cell_type;

                protected:
                    static const auto npos = static_cast<std::size_t>(-1);

                    static_string_base(TChar *buf, std::size_t cap) : vec_(reinterpret_cast<cell_type *>(buf), cap) {
                        end_string();
                    }

                    void assign(std::size_t count, TChar ch) {
                        MARSHALLING_ASSERT(count <= capacity());
                        auto countLimit = std::min(count, capacity());
                        vec_.clear();
                        std::fill_n(std::back_inserter(vec_), countLimit, ch);
                        end_string();
                    }

                    void assign(const static_string_base &other) {
                        assign(other, 0, other.size());
                    }

                    void assign(const static_string_base &other, std::size_t pos, std::size_t count) {
                        MARSHALLING_ASSERT(&other != this);
                        auto updatedCount = std::min(other.size() - pos, count);
                        auto countLimit = std::min(updatedCount, capacity());
                        vec_.clear();
                        std::copy_n(other.cbegin() + pos, countLimit, std::back_inserter(vec_));
                        end_string();
                    }

                    void assign(const TChar *str, std::size_t count) {
                        vec_.clear();
                        auto countLimit = std::min(count, capacity());
                        while ((vec_.size() < countLimit) && (*str != Ends)) {
                            vec_.push_back(*str);
                            ++str;
                        }
                        end_string();
                    }

                    void assign(const TChar *str) {
                        assign(str, capacity());
                    }

                    template<typename TIter>
                    void assign(TIter first, TIter last) {
                        vec_.assign(first, last);
                        end_string();
                    }

                    TChar &at(std::size_t pos) {
                        MARSHALLING_ASSERT(pos < size());
                        return operator[](pos);
                    }

                    const TChar &at(std::size_t pos) const {
                        MARSHALLING_ASSERT(pos < size());
                        return operator[](pos);
                    }

                    TChar &operator[](std::size_t pos) {
                        return vec_[pos];
                    }

                    const TChar &operator[](std::size_t pos) const {
                        return vec_[pos];
                    }

                    TChar &front() {
                        MARSHALLING_ASSERT(!empty());
                        return vec_.front();
                    }

                    const TChar &front() const {
                        MARSHALLING_ASSERT(!empty());
                        return vec_.front();
                    }

                    TChar &back() {
                        MARSHALLING_ASSERT(!empty());
                        return vec_[size() - 1];
                    }

                    const TChar &back() const {
                        MARSHALLING_ASSERT(!empty());
                        return vec_[size() - 1];
                    }

                    const TChar *data() const {
                        MARSHALLING_ASSERT(!vec_.empty());
                        return vec_.data();
                    }

                    TChar *begin() {
                        return vec_.begin();
                    }

                    const TChar *cbegin() const {
                        return vec_.cbegin();
                    }

                    TChar *end() {
                        return begin() + size();
                    }

                    const TChar *cend() const {
                        return cbegin() + size();
                    }

                    bool empty() const {
                        return size() == 0;
                    }

                    std::size_t size() const {
                        MARSHALLING_ASSERT(!vec_.empty());
                        return vec_.size() - 1;
                    }

                    std::size_t capacity() const {
                        return vec_.capacity() - 1;
                    }

                    void clear() {
                        vec_.clear();
                        end_string();
                    }

                    void insert(std::size_t idx, std::size_t count, TChar ch) {
                        MARSHALLING_ASSERT(idx <= size());
                        vec_.insert(vec_.begin() + idx, count, ch);
                    }

                    void insert(std::size_t idx, const TChar *str) {
                        MARSHALLING_ASSERT(idx <= size());
                        vec_.insert(vec_.begin() + idx, str, str + strlen(str));
                    }

                    void insert(std::size_t idx, const TChar *str, std::size_t count) {
                        MARSHALLING_ASSERT(idx <= size());
                        auto endStr = str + count;
                        vec_.insert(vec_.begin() + idx, str, endStr);
                    }

                    void insert(std::size_t idx, const static_string_base &other) {
                        MARSHALLING_ASSERT(idx <= size());
                        vec_.insert(vec_.begin() + idx, other.cbegin(), other.cend());
                    }

                    void insert(std::size_t idx, const static_string_base &str, std::size_t str_idx,
                                std::size_t count) {
                        MARSHALLING_ASSERT(idx <= size());
                        MARSHALLING_ASSERT(str_idx < str.size());
                        auto begIter = str.cbegin() + str_idx;
                        auto endIter = begIter + std::min((str.size() - str_idx), count);
                        vec_.insert(vec_.begin() + idx, begIter, endIter);
                    }

                    TChar *insert(const TChar *pos, TChar ch) {
                        return vec_.insert(pos, ch);
                    }

                    TChar *insert(const TChar *pos, std::size_t count, TChar ch) {
                        return vec_.insert(pos, count, ch);
                    }

                    template<typename TIter>
                    TChar *insert(const TChar *pos, TIter first, TIter last) {
                        return vec_.insert(pos, first, last);
                    }

                    void erase(std::size_t idx, std::size_t count) {
                        MARSHALLING_ASSERT(idx < size());
                        auto begIter = begin() + idx;
                        auto endIter = begIter + std::min(count, size() - idx);
                        vec_.erase(begIter, endIter);
                        MARSHALLING_ASSERT(!vec_.empty());    // Must contain '\0'
                    }

                    TChar *erase(const TChar *pos) {
                        return vec_.erase(pos, pos + 1);
                    }

                    TChar *erase(const TChar *first, const TChar *last) {
                        return vec_.erase(first, last);
                    }

                    void push_back(TChar ch) {
                        MARSHALLING_ASSERT((size() < capacity()) || (!"The string is full."));
                        vec_.insert(end(), ch);
                    }

                    void pop_back() {
                        MARSHALLING_ASSERT((!empty()) || (!"The string is empty."));
                        vec_.erase(end() - 1, end());
                    }

                    int compare(std::size_t pos1, std::size_t count1, const static_string_base &other, std::size_t pos2,
                                std::size_t count2) const {
                        MARSHALLING_ASSERT(pos1 <= size());
                        MARSHALLING_ASSERT(pos2 <= other.size());
                        count1 = std::min(count1, size() - pos1);
                        count2 = std::min(count2, other.size() - pos2);
                        auto minCount = std::min(count1, count2);
                        for (auto idx = 0U; idx < minCount; ++idx) {
                            auto thisCh = (*this)[pos1 + idx];
                            auto otherCh = other[pos2 + idx];
                            auto diff = static_cast<int>(thisCh) - static_cast<int>(otherCh);
                            if (diff != 0) {
                                return diff;
                            }
                        }

                        return static_cast<int>(count1) - static_cast<int>(count2);
                    }

                    int compare(std::size_t pos, std::size_t count, const TChar *str) const {
                        MARSHALLING_ASSERT(pos <= size());
                        count = std::min(count, size() - pos);
                        for (auto idx = 0U; idx < count; ++idx) {
                            auto ch = (*this)[pos + idx];
                            auto diff = static_cast<int>(ch) - static_cast<int>(*str);
                            if (diff != 0) {
                                return diff;
                            }

                            if (*str == Ends) {
                                return 1;
                            }
                            ++str;
                        }

                        if (*str != Ends) {
                            return 0 - static_cast<int>(*str);
                        }

                        return 0;
                    }

                    int compare(std::size_t pos1, std::size_t count1, const char *str, std::size_t count2) const {
                        MARSHALLING_ASSERT(pos1 <= size());
                        count1 = std::min(count1, size() - pos1);
                        auto minCount = std::min(count1, count2);
                        for (auto idx = 0U; idx < minCount; ++idx) {
                            auto thisCh = (*this)[pos1 + idx];
                            auto diff = static_cast<int>(thisCh) - static_cast<int>(*str);
                            if (diff != 0) {
                                return diff;
                            }

                            ++str;
                        }

                        return static_cast<int>(count1) - static_cast<int>(count2);
                    }

                    template<typename TIter>
                    void replace(const TChar *first, const TChar *last, TIter first2, TIter last2) {
                        MARSHALLING_ASSERT(first <= end());
                        MARSHALLING_ASSERT(last <= end());
                        MARSHALLING_ASSERT(first <= last);
                        auto begIter = begin() + std::distance(cbegin(), first);
                        auto endIter = begin() + std::distance(cbegin(), last);
                        for (auto iter = begIter; iter != endIter; ++iter) {
                            if (first2 == last2) {
                                vec_.erase(iter, endIter);
                                return;
                            }

                            *iter = static_cast<TChar>(*first2);
                            ++first2;
                        }

                        vec_.insert(last, first2, last2);
                    }

                    void replace(const TChar *first, const TChar *last, const TChar *str) {
                        MARSHALLING_ASSERT(first <= end());
                        MARSHALLING_ASSERT(last <= end());
                        MARSHALLING_ASSERT(first <= last);
                        auto begIter = begin() + std::distance(cbegin(), first);
                        auto endIter = begin() + std::distance(cbegin(), last);
                        for (auto iter = begIter; iter != endIter; ++iter) {
                            if (*str == Ends) {
                                vec_.erase(iter, endIter);
                                return;
                            }

                            *iter = *str;
                            ++str;
                        }

                        auto remCapacity = capacity() - size();
                        auto endStr = str + remCapacity;
                        auto lastStrIter = std::find(str, endStr, TChar(Ends));
                        vec_.insert(last, str, lastStrIter);
                    }

                    void replace(const TChar *first, const TChar *last, std::size_t count2, TChar ch) {
                        MARSHALLING_ASSERT(first <= end());
                        MARSHALLING_ASSERT(last <= end());
                        MARSHALLING_ASSERT(first <= last);
                        auto dist = static_cast<std::size_t>(std::distance(first, last));
                        auto fillDist = std::min(dist, count2);
                        auto fillIter = begin() + std::distance(cbegin(), first);
                        std::fill_n(fillIter, fillDist, ch);
                        if (count2 <= dist) {
                            vec_.erase(first + fillDist, last);
                            return;
                        }

                        vec_.insert(last, count2 - fillDist, ch);
                    }

                    std::size_t copy(TChar *dest, std::size_t count, std::size_t pos) const {
                        MARSHALLING_ASSERT(pos <= size());
                        count = std::min(count, size() - pos);
                        std::copy_n(cbegin() + pos, count, dest);
                        return count;
                    }

                    void resize(std::size_t count) {
                        resize(count, Ends);
                    }

                    void resize(std::size_t count, TChar ch) {
                        if (count <= size()) {
                            vec_.erase(cbegin() + count, cend());
                            MARSHALLING_ASSERT(vec_[size()] == Ends);
                            MARSHALLING_ASSERT(size() == count);
                            return;
                        }

                        vec_.insert(end(), count - size(), ch);
                    }

                    void swap(static_string_base &other) {
                        vec_.swap(other.vec_);
                    }

                    std::size_t find(const TChar *str, std::size_t pos, std::size_t count) const {
                        MARSHALLING_ASSERT(pos <= size());
                        auto remCount = size() - pos;
                        if (remCount < count) {
                            return npos;
                        }

                        auto maxPos = size() - count;
                        for (auto idx = pos; idx <= maxPos; ++idx) {
                            auto thisStrBeg = &vec_[idx];
                            auto thisStrEnd = thisStrBeg + count;
                            if (std::equal(thisStrBeg, thisStrEnd, str)) {
                                return idx;
                            }
                        }
                        return npos;
                    }

                    std::size_t find(const TChar *str, std::size_t pos) const {
                        MARSHALLING_ASSERT(pos <= size());
                        auto maxStrCount = size() - pos;
                        auto maxStrEnd = str + maxStrCount;
                        auto iter = std::find(str, maxStrEnd, TChar(Ends));
                        if (iter == maxStrEnd) {
                            return npos;
                        }

                        auto strCount = static_cast<std::size_t>(std::distance(str, iter));
                        return find(str, pos, strCount);
                    }

                    std::size_t find(TChar ch, std::size_t pos) const {
                        MARSHALLING_ASSERT(pos <= size());
                        auto begIter = cbegin() + pos;
                        auto iter = std::find(begIter, cend(), ch);
                        if (iter == cend()) {
                            return npos;
                        }

                        return static_cast<std::size_t>(std::distance(cbegin(), iter));
                    }

                    std::size_t rfind(const TChar *str, std::size_t pos, std::size_t count) const {
                        if ((empty()) || (size() < count)) {
                            return npos;
                        }

                        pos = std::min(pos, size() - 1);
                        auto startIdx = static_cast<int>(std::min(pos, size() - count));
                        for (auto idx = startIdx; 0 <= idx; --idx) {
                            auto thisStrBeg = &vec_[idx];
                            auto thisStrEnd = thisStrBeg + count;
                            if (std::equal(thisStrBeg, thisStrEnd, str)) {
                                return static_cast<std::size_t>(idx);
                            }
                        }
                        return npos;
                    }

                    std::size_t rfind(const TChar *str, std::size_t pos) const {
                        return rfind(str, pos, strlen(str));
                    }

                    std::size_t rfind(TChar ch, std::size_t pos) const {
                        if (empty()) {
                            return npos;
                        }

                        pos = std::min(pos, size() - 1);
                        auto begIter = std::reverse_iterator<const TChar *>(cbegin() + pos + 1);
                        auto endIter = std::reverse_iterator<const TChar *>(cbegin());
                        MARSHALLING_ASSERT(static_cast<std::size_t>(std::distance(begIter, endIter)) == (pos + 1));
                        auto iter = std::find(begIter, endIter, ch);
                        if (iter == endIter) {
                            return npos;
                        }

                        return static_cast<std::size_t>(std::distance(iter, endIter)) - 1U;
                    }

                    std::size_t find_first_of(const TChar *str, std::size_t pos, std::size_t count) const {
                        if (empty()) {
                            return npos;
                        }

                        pos = std::min(pos, size() - 1);
                        auto endStr = str + count;
                        for (auto iter = cbegin() + pos; iter != cend(); ++iter) {
                            auto foundIter = std::find(str, endStr, *iter);
                            if (foundIter != endStr) {
                                return static_cast<std::size_t>(std::distance(cbegin(), iter));
                            }
                        }

                        return npos;
                    }

                    std::size_t find_first_of(const TChar *str, std::size_t pos) const {
                        return find_first_of(str, pos, strlen(str));
                    }

                    std::size_t find_first_not_of(const TChar *str, std::size_t pos, std::size_t count) const {
                        if (empty()) {
                            return npos;
                        }

                        pos = std::min(pos, size() - 1);
                        auto endStr = str + count;
                        for (auto iter = cbegin() + pos; iter != cend(); ++iter) {
                            auto found = std::none_of(str, endStr, [iter](TChar ch) -> bool { return *iter == ch; });

                            if (found) {
                                return static_cast<std::size_t>(std::distance(cbegin(), iter));
                            }
                        }

                        return npos;
                    }

                    std::size_t find_first_not_of(const TChar *str, std::size_t pos) const {
                        return find_first_not_of(str, pos, strlen(str));
                    }

                    std::size_t find_first_not_of(TChar ch, std::size_t pos) const {
                        if (empty()) {
                            return npos;
                        }

                        pos = std::min(pos, size() - 1);
                        auto iter
                            = std::find_if(cbegin() + pos, cend(), [ch](TChar nextCh) -> bool { return ch != nextCh; });

                        if (iter == cend()) {
                            return npos;
                        }

                        return static_cast<std::size_t>(std::distance(cbegin(), iter));
                    }

                    std::size_t find_last_of(const TChar *str, std::size_t pos, std::size_t count) const {
                        if (empty()) {
                            return npos;
                        }

                        pos = std::min(pos, size() - 1);
                        auto endStr = str + count;

                        auto begIter = std::reverse_iterator<const TChar *>(cbegin() + pos + 1);
                        auto endIter = std::reverse_iterator<const TChar *>(cbegin());
                        for (auto iter = begIter; iter != endIter; ++iter) {
                            auto foundIter = std::find(str, endStr, *iter);
                            if (foundIter != endStr) {
                                return static_cast<std::size_t>(std::distance(iter, endIter)) - 1U;
                            }
                        }

                        return npos;
                    }

                    std::size_t find_last_of(const TChar *str, std::size_t pos) const {
                        return find_last_of(str, pos, strlen(str));
                    }

                    std::size_t find_last_not_of(const TChar *str, std::size_t pos, std::size_t count) const {
                        if (empty()) {
                            return npos;
                        }

                        pos = std::min(pos, size() - 1);
                        auto endStr = str + count;
                        auto begIter = std::reverse_iterator<const TChar *>(cbegin() + pos + 1);
                        auto endIter = std::reverse_iterator<const TChar *>(cbegin());
                        for (auto iter = begIter; iter != endIter; ++iter) {
                            auto found = std::none_of(str, endStr, [iter](TChar ch) -> bool { return *iter == ch; });

                            if (found) {
                                return static_cast<std::size_t>(std::distance(iter, endIter)) - 1U;
                            }
                        }

                        return npos;
                    }

                    std::size_t find_last_not_of(const TChar *str, std::size_t pos) const {
                        return find_last_not_of(str, pos, strlen(str));
                    }

                    std::size_t find_last_not_of(TChar ch, std::size_t pos) const {
                        if (empty()) {
                            return npos;
                        }

                        pos = std::min(pos, size() - 1);
                        auto begIter = std::reverse_iterator<const TChar *>(cbegin() + pos + 1);
                        auto endIter = std::reverse_iterator<const TChar *>(cbegin());
                        auto iter = std::find_if(begIter, endIter, [ch](TChar nextCh) -> bool { return ch != nextCh; });

                        if (iter == endIter) {
                            return npos;
                        }

                        return static_cast<std::size_t>(std::distance(iter, endIter)) - 1U;
                    }

                    bool operator<(const TChar *str) const {
                        for (auto idx = 0U; idx < size(); ++idx) {
                            if (*str == Ends) {
                                return false;
                            }

                            auto ch = vec_[idx];

                            if (ch < *str) {
                                return true;
                            }

                            if (ch != *str) {
                                break;
                            }

                            ++str;
                        }
                        return false;
                    }

                    bool operator>(const TChar *str) const {
                        for (auto idx = 0U; idx < size(); ++idx) {
                            if (*str == Ends) {
                                return true;
                            }

                            auto ch = vec_[idx];
                            if (*str < ch) {
                                return true;
                            }

                            if (ch != *str) {
                                break;
                            }

                            ++str;
                        }
                        return false;
                    }

                    bool operator==(const TChar *str) const {
                        for (auto idx = 0U; idx < size(); ++idx) {
                            if (*str == Ends) {
                                return false;
                            }

                            auto ch = vec_[idx];
                            if (*str != ch) {
                                return false;
                            }

                            ++str;
                        }

                        return true;
                    }

                private:
                    void end_string() {
                        vec_.push_back(TChar(Ends));
                    }

                    std::size_t strlen(const TChar *str) const {
                        auto *strTmp = str;
                        while (*strTmp != Ends) {
                            ++strTmp;
                        }
                        return static_cast<std::size_t>(std::distance(str, strTmp));
                    }

                    static const TChar Ends = static_cast<TChar>('\0');
                    static_vector_base<TChar> vec_;
                };

                template<typename TChar, std::size_t TSize>
                struct static_string_storage_base {
                    using storage_type = std::array<TChar, TSize>;
                    storage_type data_;
                };

            }    // namespace detail

            /// @brief Replacement to <a href="http://en.cppreference.com/w/cpp/string/basic_string">std::string</a>
            ///     when no dynamic memory allocation is allowed.
            /// @details Uses <a href="http://en.cppreference.com/w/cpp/container/array">std::array</a>
            ///     in its private members to store the zero-terminated string. Provides
            ///     almost the same interface as
            ///     <a href="http://en.cppreference.com/w/cpp/string/basic_string">std::string</a>.
            /// @tparam TSize Maximum length of the string, not including zero termination character.
            /// @tparam Type of the single character.
            /// @headerfile "marshalling/container/static_string.h"
            template<std::size_t TSize, typename TChar = char>
            class static_string : public detail::static_string_storage_base<TChar, TSize + 1>,
                                  public detail::static_string_base<TChar> {
                using storage_base_type = detail::static_string_storage_base<TChar, TSize + 1>;
                using base_type = detail::static_string_base<TChar>;

            public:
                /// @brief Type of single character.
                using value_type = TChar;
                /// @brief Type used for size information
                using size_type = std::size_t;
                /// @brief Type used in pointer arithmetics
                using difference_type = typename storage_base_type::storage_type::difference_type;
                /// @brief Reference to single character
                using reference = value_type &;
                /// @brief Const reference to single character
                using const_reference = const value_type &;
                /// @brief Pointer to single character
                using pointer = value_type *;
                /// @brief Const pointer to single character
                using const_pointer = const value_type *;
                /// @brief Type of the iterator.
                using iterator = pointer;
                /// @brief Type of the const iterator
                using const_iterator = const_pointer;
                /// @brief Type of the reverse iterator
                using reverse_iterator = std::reverse_iterator<iterator>;
                /// @brief Type of the const reverse iterator
                using const_reverse_iterator = std::reverse_iterator<const_iterator>;

                /// @brief Same as std::string::npos.
                static const decltype(base_type::npos) npos = base_type::npos;

                /// @brief Default constructor
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/basic_string">Reference</a>
                static_string() : base_type(&storage_base_type::data_[0], storage_base_type::data_.size()) {
                }

                /// @brief Constructor variant
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/basic_string">Reference</a>
                static_string(size_type count, value_type ch) :
                    base_type(&storage_base_type::data_[0], storage_base_type::data_.size()) {
                    assign(count, ch);
                }

                /// @brief Constructor variant.
                /// @details Allows reception of any other static_string with any size.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/basic_string">Reference</a>
                template<std::size_t TOtherSize>
                static_string(const static_string<TOtherSize, TChar> &other, size_type pos, size_type count = npos) :
                    base_type(&storage_base_type::data_[0], storage_base_type::data_.size()) {
                    assign(other, pos, count);
                }

                /// @brief Constructor variant.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/basic_string">Reference</a>
                static_string(const_pointer str, size_type count) :
                    base_type(&storage_base_type::data_[0], storage_base_type::data_.size()) {
                    assign(str, count);
                }

                /// @brief Constructor variant.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/basic_string">Reference</a>
                static_string(const_pointer str) :
                    base_type(&storage_base_type::data_[0], storage_base_type::data_.size()) {
                    assign(str);
                }

                /// @brief Constructor variant.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/basic_string">Reference</a>
                template<typename TIter>
                static_string(TIter first, TIter last) :
                    base_type(&storage_base_type::data_[0], storage_base_type::data_.size()) {
                    assign(first, last);
                }

                /// @brief Copy constructor.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/basic_string">Reference</a>
                static_string(const static_string &other) :
                    base_type(&storage_base_type::data_[0], storage_base_type::data_.size()) {
                    assign(other);
                }

                /// @brief Copy constructor variant.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/basic_string">Reference</a>
                template<std::size_t TOtherSize>
                explicit static_string(const static_string<TOtherSize, TChar> &other) :
                    base_type(&storage_base_type::data_[0], storage_base_type::data_.size()) {
                    assign(other);
                }

                /// @brief Constructor variant.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/basic_string">Reference</a>
                static_string(std::initializer_list<value_type> init) :
                    base_type(&storage_base_type::data_[0], storage_base_type::data_.size()) {
                    assign(init.begin(), init.end());
                }

                /// @brief Copy assignment
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator%3D">Reference</a>
                static_string &operator=(const static_string &other) {
                    return assign(other);
                }

                /// @brief Copy assignment from string of different capacity.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator%3D">Reference</a>
                template<std::size_t TOtherSize>
                static_string &operator=(const static_string<TOtherSize, TChar> &other) {
                    return assign(other);
                }

                /// @brief Assignment operator
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator%3D">Reference</a>
                static_string &operator=(const_pointer str) {
                    return assign(str);
                }

                /// @brief Assignment operator
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator%3D">Reference</a>
                static_string &operator=(value_type ch) {
                    return assign(1, ch);
                }

                /// @brief Assignment operator
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator%3D">Reference</a>
                static_string &operator=(std::initializer_list<value_type> init) {
                    return assign(init);
                }

                /// @brief Assign characters to a string
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/assign">Reference</a>
                static_string &assign(size_type count, value_type ch) {
                    base_type::assign(count, ch);
                    return *this;
                }

                /// @brief Assign characters to a string
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/assign">Reference</a>
                template<typename TOtherSize>
                static_string &assign(const static_string &other) {
                    if (&other != this) {
                        base_type::assign(other);
                    }
                    return *this;
                }

                /// @brief Assign characters to a string
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/assign">Reference</a>
                template<std::size_t TOtherSize>
                static_string &assign(const static_string<TOtherSize, TChar> &other) {
                    base_type::assign(other);
                    return *this;
                }

                /// @brief Assign characters to a string
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/assign">Reference</a>
                template<std::size_t TOtherSize>
                static_string &assign(const static_string<TOtherSize, TChar> &other, size_type pos,
                                      size_type count = npos) {
                    base_type::assign(other, pos, count);
                    return *this;
                }

                /// @brief Assign characters to a string
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/assign">Reference</a>
                static_string &assign(const_pointer str, size_type count) {
                    base_type::assign(str, count);
                    return *this;
                }

                /// @brief Assign characters to a string
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/assign">Reference</a>
                static_string &assign(const_pointer str) {
                    base_type::assign(str);
                    return *this;
                }

                /// @brief Assign characters to a string
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/assign">Reference</a>
                template<typename TIter>
                static_string &assign(TIter first, TIter last) {
                    base_type::assign(first, last);
                    return *this;
                }

                /// @brief Assign characters to a string
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/assign">Reference</a>
                static_string &assign(std::initializer_list<value_type> init) {
                    return assign(init.begin(), init.end());
                }

                /// @brief Access specified character with bounds checking.
                /// @details The bounds are check with assertion (using MARSHALLING_ASSERT()).
                ///     When compiled with NDEBUG definition, equivalent to operator[]().
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/at">Reference</a>
                reference at(size_type pos) {
                    return base_type::at(pos);
                }

                /// @brief Access specified character with bounds checking.
                /// @details The bounds are check with assertion (using MARSHALLING_ASSERT()).
                ///     When compiled with NDEBUG definition, equivalent to operator[]().
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/at">Reference</a>
                const_reference at(size_type pos) const {
                    return base_type::at(pos);
                }

                /// @brief Access specified character without bounds checking.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_at">Reference</a>
                reference operator[](size_type pos) {
                    return base_type::operator[](pos);
                }

                /// @brief Access specified character without bounds checking.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_at">Reference</a>
                const_reference operator[](size_type pos) const {
                    return base_type::operator[](pos);
                }

                /// @brief Accesses the first character.
                /// @pre The string mustn't be empty.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/front">Reference</a>
                reference front() {
                    return base_type::front();
                }

                /// @brief Accesses the first character.
                /// @pre The string mustn't be empty.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/front">Reference</a>
                const_reference front() const {
                    return base_type::front();
                }

                /// @brief Accesses the last character.
                /// @pre The string mustn't be empty.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/back">Reference</a>
                reference back() {
                    return base_type::back();
                }

                /// @brief Accesses the last character.
                /// @pre The string mustn't be empty.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/back">Reference</a>
                const_reference back() const {
                    return base_type::back();
                }

                /// @brief Returns a pointer to the first character of a string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/data">Reference</a>
                const_pointer data() const {
                    return base_type::data();
                }

                /// @brief Returns a non-modifiable standard C character array version of the string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/c_str">Reference</a>
                const_pointer c_str() const {
                    return data();
                }

                /// @brief Returns an iterator to the beginning.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/begin">Reference</a>
                iterator begin() {
                    return base_type::begin();
                }

                /// @brief Returns an iterator to the beginning.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/begin">Reference</a>
                const_iterator begin() const {
                    return cbegin();
                }

                /// @brief Returns an iterator to the beginning.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/begin">Reference</a>
                const_iterator cbegin() const {
                    return base_type::cbegin();
                }

                /// @brief Returns an iterator to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/end">Reference</a>
                iterator end() {
                    return base_type::end();
                }

                /// @brief Returns an iterator to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/end">Reference</a>
                const_iterator end() const {
                    return cend();
                }

                /// @brief Returns an iterator to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/end">Reference</a>
                const_iterator cend() const {
                    return base_type::cend();
                }

                /// @brief Returns a reverse iterator to the beginning.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/rbegin">Reference</a>
                reverse_iterator rbegin() {
                    return reverse_iterator(end());
                }

                /// @brief Returns a reverse iterator to the beginning.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/rbegin">Reference</a>
                const_reverse_iterator rbegin() const {
                    return crbegin();
                }

                /// @brief Returns a reverse iterator to the beginning.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/rbegin">Reference</a>
                const_reverse_iterator crbegin() const {
                    return reverse_iterator(cend());
                }

                /// @brief Returns a reverse iterator to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/rend">Reference</a>
                reverse_iterator rend() {
                    return reverse_iterator(begin());
                }

                /// @brief Returns a reverse iterator to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/rend">Reference</a>
                const_reverse_iterator rend() const {
                    return crend();
                }

                /// @brief Returns a reverse iterator to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/rend">Reference</a>
                const_reverse_iterator crend() const {
                    return reverse_iterator(cbegin());
                }

                /// @brief Checks whether the string is empty.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/empty">Reference</a>
                bool empty() const {
                    return base_type::empty();
                }

                /// @brief returns the number of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/size">Reference</a>
                size_type size() const {
                    return base_type::size();
                }

                /// @brief returns the number of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/size">Reference</a>
                size_type length() const {
                    return size();
                }

                /// @brief Returns the maximum number of characters.
                /// @details Same as capacity().
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/max_size">Reference</a>
                size_type max_size() const {
                    return capacity();
                }

                /// @brief Reserves storage.
                /// @details Does nothing for static string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/reserve">Reference</a>
                void reserve(size_type) {
                }

                /// @brief returns the number of characters that can be held in currently allocated storage.
                /// @details Returns TSize provided as a template argument to this class.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/capacity">Reference</a>
                size_type capacity() const {
                    return base_type::capacity();
                }

                /// @brief Reduces memory usage by freeing unused memory.
                /// @details Does nothing for static string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/shrink_to_fit">Reference</a>
                void shrink_to_fit() {
                }

                /// @brief Clears the contents.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/clear">Reference</a>
                void clear() {
                    base_type::clear();
                }

                /// @brief Inserts characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/insert">Reference</a>
                static_string &insert(size_type idx, size_type count, value_type ch) {
                    base_type::insert(idx, count, ch);
                    return *this;
                }

                /// @brief Inserts characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/insert">Reference</a>
                static_string &insert(size_type idx, const_pointer str) {
                    base_type::insert(idx, str);
                    return *this;
                }

                /// @brief Inserts characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/insert">Reference</a>
                static_string &insert(size_type idx, const_pointer str, size_type count) {
                    base_type::insert(idx, str, count);
                    return *this;
                }

                /// @brief Inserts characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/insert">Reference</a>
                template<std::size_t TAnySize>
                static_string &insert(size_type idx, const static_string<TAnySize, TChar> &str) {
                    base_type::insert(idx, str);
                    return *this;
                }

                /// @brief Inserts characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/insert">Reference</a>
                template<std::size_t TAnySize>
                static_string &insert(size_type idx, const static_string<TAnySize, TChar> &str, size_type str_idx,
                                      size_type count = npos) {
                    base_type::insert(idx, str, str_idx, count);
                    return *this;
                }

                /// @brief Inserts characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/insert">Reference</a>
                iterator insert(const_iterator pos, value_type ch) {
                    return base_type::insert(pos, ch);
                }

                /// @brief Inserts characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/insert">Reference</a>
                iterator insert(const_iterator pos, size_type count, value_type ch) {
                    return base_type::insert(pos, count, ch);
                }

                /// @brief Inserts characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/insert">Reference</a>
                template<typename TIter>
                iterator insert(const_iterator pos, TIter first, TIter last) {
                    return base_type::insert(pos, first, last);
                }

                /// @brief Inserts characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/insert">Reference</a>
                iterator insert(const_iterator pos, std::initializer_list<value_type> init) {
                    return insert(pos, init.begin(), init.end());
                }

                /// @brief Removes characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/erase">Reference</a>
                static_string &erase(std::size_t idx, std::size_t count = npos) {
                    base_type::erase(idx, count);
                    return *this;
                }

                /// @brief Removes characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/erase">Reference</a>
                iterator erase(const_iterator pos) {
                    return base_type::erase(pos);
                }

                /// @brief Removes characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/erase">Reference</a>
                iterator erase(const_iterator first, const_iterator last) {
                    return base_type::erase(first, last);
                }

                /// @brief Appends a character to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/push_back">Reference</a>
                void push_back(value_type ch) {
                    base_type::push_back(ch);
                }

                /// @brief Removes the last character.
                /// @pre The string mustn't be empty.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/pop_back">Reference</a>
                void pop_back() {
                    base_type::pop_back();
                }

                /// @brief Appends characters to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/append">Reference</a>
                static_string &append(size_type count, value_type ch) {
                    return insert(size(), count, ch);
                }

                /// @brief Appends characters to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/append">Reference</a>
                template<std::size_t TAnySize>
                static_string &append(const static_string<TAnySize, TChar> &other) {
                    return insert(size(), other);
                }

                /// @brief Appends characters to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/append">Reference</a>
                template<std::size_t TAnySize>
                static_string &append(const static_string<TAnySize, TChar> &other, size_type pos,
                                      size_type count = npos) {
                    return insert(size(), other, pos, count);
                }

                /// @brief Appends characters to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/append">Reference</a>
                static_string &append(const TChar *str, size_type count) {
                    return insert(size(), str, count);
                }

                /// @brief Appends characters to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/append">Reference</a>
                static_string &append(const TChar *str) {
                    return insert(size(), str);
                }

                /// @brief Appends characters to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/append">Reference</a>
                template<typename TIter>
                static_string &append(TIter first, TIter last) {
                    insert(end(), first, last);
                    return *this;
                }

                /// @brief Appends characters to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/append">Reference</a>
                static_string &append(std::initializer_list<value_type> init) {
                    insert(end(), init.begin(), init.end());
                    return *this;
                }

                /// @brief Appends characters to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator%2B%3D">Reference</a>
                template<std::size_t TAnySize>
                static_string &operator+=(const static_string<TAnySize, TChar> &other) {
                    return append(other);
                }

                /// @brief Appends characters to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator%2B%3D">Reference</a>
                static_string &operator+=(value_type ch) {
                    return append(1U, ch);
                }

                /// @brief Appends characters to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator%2B%3D">Reference</a>
                static_string &operator+=(const_pointer str) {
                    return append(str);
                }

                /// @brief Appends characters to the end.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator%2B%3D">Reference</a>
                static_string &operator+=(std::initializer_list<value_type> init) {
                    return append(init);
                }

                /// @brief Compares two strings.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/compare">Reference</a>
                template<std::size_t TAnySize>
                int compare(const static_string<TAnySize, TChar> &other) const {
                    return compare(0, size(), other);
                }

                /// @brief Compares two strings.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/compare">Reference</a>
                template<std::size_t TAnySize>
                int compare(size_type pos, size_type count, const static_string<TAnySize, TChar> &other) const {
                    return compare(pos, count, other, 0, other.size());
                }

                /// @brief Compares two strings.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/compare">Reference</a>
                template<std::size_t TAnySize>
                int compare(size_type pos1, size_type count1, const static_string<TAnySize, TChar> &other,
                            size_type pos2, size_type count2 = npos) const {
                    return base_type::compare(pos1, count1, other, pos2, count2);
                }

                /// @brief Compares two strings.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/compare">Reference</a>
                int compare(const_pointer str) const {
                    return compare(0, size(), str);
                }

                /// @brief Compares two strings.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/compare">Reference</a>
                int compare(size_type pos, size_type count, const_pointer str) const {
                    return base_type::compare(pos, count, str);
                }

                /// @brief Compares two strings.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/compare">Reference</a>
                int compare(size_type pos, size_type count1, const_pointer str, size_type count2) const {
                    return base_type::compare(pos, count1, str, count2);
                }

                /// @brief Replaces specified portion of a string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/replace">Reference</a>
                template<std::size_t TAnySize>
                static_string &replace(size_type pos, size_type count, const static_string<TAnySize, TChar> &other) {
                    MARSHALLING_ASSERT(pos <= size());
                    auto begIter = begin() + pos;
                    auto remCount = static_cast<std::size_t>(std::distance(begIter, end()));
                    auto endIter = begIter + std::min(count, remCount);
                    return replace(begIter, endIter, other.begin(), other.end());
                }

                /// @brief Replaces specified portion of a string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/replace">Reference</a>
                template<std::size_t TAnySize>
                static_string &replace(const_iterator first, const_iterator last,
                                       const static_string<TAnySize, TChar> &other) {
                    return replace(first, last, other.begin(), other.end());
                }

                /// @brief Replaces specified portion of a string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/replace">Reference</a>
                template<std::size_t TAnySize>
                static_string &replace(size_type pos, size_type count, const static_string<TAnySize, TChar> &other,
                                       size_type pos2, size_type count2 = npos) {
                    MARSHALLING_ASSERT(pos <= size());
                    auto begIter = begin() + pos;
                    auto remCount = static_cast<std::size_t>(std::distance(begIter, end()));
                    auto endIter = begIter + std::min(count, remCount);

                    MARSHALLING_ASSERT(pos2 <= other.size());
                    auto begIter2 = other.begin() + pos2;
                    auto remCount2 = static_cast<std::size_t>(std::distance(begIter2, other.end()));
                    auto endIter2 = begIter2 + std::min(count2, remCount2);

                    return replace(begIter, endIter, begIter2, endIter2);
                }

                /// @brief Replaces specified portion of a string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/replace">Reference</a>
                template<typename TIter>
                static_string &replace(const_iterator first, const_iterator last, TIter first2, TIter last2) {
                    base_type::replace(first, last, first2, last2);
                    return *this;
                }

                /// @brief Replaces specified portion of a string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/replace">Reference</a>
                static_string &replace(size_type pos, size_type count, const_pointer str, size_type count2) {
                    MARSHALLING_ASSERT(pos <= size());
                    auto begIter = cbegin() + pos;
                    auto endIter = begIter + std::min(count, size() - pos);
                    return replace(begIter, endIter, str, str + count2);
                }

                /// @brief Replaces specified portion of a string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/replace">Reference</a>
                static_string &replace(const_iterator first, const_iterator last, const_pointer str, size_type count2) {
                    return replace(first, last, str, str + count2);
                }

                /// @brief Replaces specified portion of a string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/replace">Reference</a>
                static_string &replace(size_type pos, size_type count, const_pointer str) {
                    MARSHALLING_ASSERT(pos <= size());
                    auto begIter = cbegin() + pos;
                    auto endIter = begIter + std::min(count, size() - pos);
                    return replace(begIter, endIter, str);
                }

                /// @brief Replaces specified portion of a string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/replace">Reference</a>
                static_string &replace(const_iterator first, const_iterator last, const_pointer str) {
                    base_type::replace(first, last, str);
                    return *this;
                }

                /// @brief Replaces specified portion of a string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/replace">Reference</a>
                static_string &replace(size_type pos, size_type count, size_type count2, value_type ch) {
                    MARSHALLING_ASSERT(pos <= size());
                    auto begIter = cbegin() + pos;
                    auto endIter = begIter + std::min(count, size() - pos);
                    return replace(begIter, endIter, count2, ch);
                }

                /// @brief Replaces specified portion of a string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/replace">Reference</a>
                static_string &replace(const_iterator first, const_iterator last, size_type count2, value_type ch) {
                    base_type::replace(first, last, count2, ch);
                    return *this;
                }

                /// @brief Replaces specified portion of a string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/replace">Reference</a>
                static_string &replace(const_iterator first, const_iterator last,
                                       std::initializer_list<value_type> init) {
                    return replace(first, last, init.begin(), init.end());
                }

                /// @brief Returns a substring.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/substr">Reference</a>
                static_string substr(size_type pos = 0, size_type count = npos) const {
                    MARSHALLING_ASSERT(pos <= size());
                    auto begIter = cbegin() + pos;
                    auto endIter = begIter + std::min(count, size() - pos);
                    return static_string(cbegin() + pos, endIter);
                }

                /// @brief Copies characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/copy">Reference</a>
                size_type copy(pointer dest, size_type count, size_type pos = 0) const {
                    return base_type::copy(dest, count, pos);
                }

                /// @brief Changes the number of characters stored.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/resize">Reference</a>
                void resize(size_type count) {
                    base_type::resize(count);
                }

                /// @brief Changes the number of characters stored.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/resize">Reference</a>
                void resize(size_type count, value_type ch) {
                    base_type::resize(count, ch);
                }

                /// @brief Swaps the contents of two strings.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/swap">Reference</a>
                template<std::size_t TAnySize>
                void swap(static_string<TAnySize, TChar> &other) {
                    base_type::swap(other);
                }

                /// @brief Find characters in the string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find">Reference</a>
                template<std::size_t TAnySize>
                size_type find(const static_string<TAnySize, TChar> &str, size_type pos = 0) const {
                    MARSHALLING_ASSERT(pos <= size());
                    return find(str.cbegin(), pos, str.size());
                }

                /// @brief Find characters in the string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find">Reference</a>
                size_type find(const_pointer str, size_type pos, size_type count) const {
                    return base_type::find(str, pos, count);
                }

                /// @brief Find characters in the string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find">Reference</a>
                size_type find(const_pointer str, size_type pos = 0) const {
                    return base_type::find(str, pos);
                }

                /// @brief Find characters in the string.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find">Reference</a>
                size_type find(value_type ch, size_type pos = 0) const {
                    return base_type::find(ch, pos);
                }

                /// @brief Find the last occurrence of the substring.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/rfind">Reference</a>
                template<std::size_t TAnySize>
                size_type rfind(const static_string<TAnySize, TChar> &str, size_type pos = npos) const {
                    return rfind(str.cbegin(), pos, str.size());
                }

                /// @brief Find the last occurrence of the substring.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/rfind">Reference</a>
                size_type rfind(const_pointer str, size_type pos, size_type count) const {
                    return base_type::rfind(str, pos, count);
                }

                /// @brief Find the last occurrence of the substring.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/rfind">Reference</a>
                size_type rfind(const_pointer str, size_type pos = npos) const {
                    return base_type::rfind(str, pos);
                }

                /// @brief Find the last occurrence of the substring.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/rfind">Reference</a>
                size_type rfind(value_type ch, size_type pos = npos) const {
                    return base_type::rfind(ch, pos);
                }

                /// @brief Find first occurrence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_first_of">Reference</a>
                template<std::size_t TAnySize>
                size_type find_first_of(const static_string<TAnySize, TChar> &str, size_type pos = 0) const {
                    MARSHALLING_ASSERT(pos <= size());
                    return find_first_of(str.cbegin(), pos, str.size());
                }

                /// @brief Find first occurrence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_first_of">Reference</a>
                size_type find_first_of(const_pointer str, size_type pos, size_type count) const {
                    return base_type::find_first_of(str, pos, count);
                }

                /// @brief Find first occurrence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_first_of">Reference</a>
                size_type find_first_of(const_pointer str, size_type pos = 0) const {
                    return base_type::find_first_of(str, pos);
                }

                /// @brief Find first occurrence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_first_of">Reference</a>
                size_type find_first_of(value_type ch, size_type pos = 0) const {
                    return find(ch, pos);
                }

                /// @brief Find first absence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_first_not_of">Reference</a>
                template<std::size_t TAnySize>
                size_type find_first_not_of(const static_string<TAnySize, TChar> &str, size_type pos = 0) const {
                    MARSHALLING_ASSERT(pos <= size());
                    return find_first_not_of(str.cbegin(), pos, str.size());
                }

                /// @brief Find first absence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_first_not_of">Reference</a>
                size_type find_first_not_of(const_pointer str, size_type pos, size_type count) const {
                    return base_type::find_first_not_of(str, pos, count);
                }

                /// @brief Find first absence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_first_not_of">Reference</a>
                size_type find_first_not_of(const_pointer str, size_type pos = 0) const {
                    return base_type::find_first_not_of(str, pos);
                }

                /// @brief Find first absence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_first_not_of">Reference</a>
                size_type find_first_not_of(value_type ch, size_type pos = 0) const {
                    return base_type::find_first_not_of(ch, pos);
                }

                /// @brief Find last occurrence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_last_of">Reference</a>
                template<std::size_t TAnySize>
                size_type find_last_of(const static_string<TAnySize, TChar> &str, size_type pos = npos) const {
                    return find_last_of(str.cbegin(), pos, str.size());
                }

                /// @brief Find last occurrence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_last_of">Reference</a>
                size_type find_last_of(const_pointer str, size_type pos, size_type count) const {
                    return base_type::find_last_of(str, pos, count);
                }

                /// @brief Find last occurrence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_last_of">Reference</a>
                size_type find_last_of(const_pointer str, size_type pos = npos) const {
                    return base_type::find_last_of(str, pos);
                }

                /// @brief Find last occurrence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_last_of">Reference</a>
                size_type find_last_of(value_type ch, size_type pos = npos) const {
                    return rfind(ch, pos);
                }

                /// @brief Find last absence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_last_not_of">Reference</a>
                template<std::size_t TAnySize>
                size_type find_last_not_of(const static_string<TAnySize, TChar> &str, size_type pos = npos) const {
                    return find_last_not_of(str.cbegin(), pos, str.size());
                }

                /// @brief Find last absence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_last_not_of">Reference</a>
                size_type find_last_not_of(const_pointer str, size_type pos, size_type count) const {
                    return base_type::find_last_not_of(str, pos, count);
                }

                /// @brief Find last absence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_last_not_of">Reference</a>
                size_type find_last_not_of(const_pointer str, size_type pos = npos) const {
                    return base_type::find_last_not_of(str, pos);
                }

                /// @brief Find last absence of characters.
                /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/find_last_not_of">Reference</a>
                size_type find_last_not_of(value_type ch, size_type pos = npos) const {
                    return base_type::find_last_not_of(ch, pos);
                }

                /// @brief Lexicographical compare to other string
                bool operator<(const_pointer str) const {
                    return base_type::operator<(str);
                }

                /// @brief Lexicographical compare to other string
                bool operator>(const_pointer str) const {
                    return base_type::operator>(str);
                }

                /// @brief Lexicographical compare to other string
                bool operator==(const_pointer str) const {
                    return base_type::operator==(str);
                }
            };

            /// @brief Lexicographical compare between the strings.
            /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_cmp">Reference</a>
            /// @related static_string
            template<std::size_t TSize1, std::size_t TSize2, typename TChar>
            bool operator<(const static_string<TSize1, TChar> &str1, const static_string<TSize2, TChar> &str2) {
                return std::lexicographical_compare(str1.begin(), str1.end(), str2.begin(), str2.end());
            }

            /// @brief Lexicographical compare between the strings.
            /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_cmp">Reference</a>
            /// @related static_string
            template<std::size_t TSize1, typename TChar>
            bool operator<(const TChar *str1, const static_string<TSize1, TChar> &str2) {
                return (str2 > str1);
            }

            /// @brief Lexicographical compare between the strings.
            /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_cmp">Reference</a>
            /// @related static_string
            template<std::size_t TSize1, std::size_t TSize2, typename TChar>
            bool operator<=(const static_string<TSize1, TChar> &str1, const static_string<TSize2, TChar> &str2) {
                return !(str2 < str1);
            }

            /// @brief Lexicographical compare between the strings.
            /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_cmp">Reference</a>
            /// @related static_string
            template<std::size_t TSize1, typename TChar>
            bool operator<=(const TChar *str1, const static_string<TSize1, TChar> &str2) {
                return !(str2 < str1);
            }

            /// @brief Lexicographical compare between the strings.
            /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_cmp">Reference</a>
            /// @related static_string
            template<std::size_t TSize1, std::size_t TSize2, typename TChar>
            bool operator>(const static_string<TSize1, TChar> &str1, const static_string<TSize2, TChar> &str2) {
                return (str2 < str1);
            }

            /// @brief Lexicographical compare between the strings.
            /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_cmp">Reference</a>
            /// @related static_string
            template<std::size_t TSize1, typename TChar>
            bool operator>(const TChar *str1, const static_string<TSize1, TChar> &str2) {
                return (str2 < str1);
            }

            /// @brief Lexicographical compare between the strings.
            /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_cmp">Reference</a>
            /// @related static_string
            template<std::size_t TSize1, std::size_t TSize2, typename TChar>
            bool operator>=(const static_string<TSize1, TChar> &str1, const static_string<TSize2, TChar> &str2) {
                return !(str1 < str2);
            }

            /// @brief Lexicographical compare between the strings.
            /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_cmp">Reference</a>
            /// @related static_string
            template<std::size_t TSize1, typename TChar>
            bool operator>=(const TChar *str1, const static_string<TSize1, TChar> &str2) {
                return !(str1 < str2);
            }

            /// @brief Lexicographical compare between the strings.
            /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_cmp">Reference</a>
            /// @related static_string
            template<std::size_t TSize1, typename TChar>
            bool operator>=(const static_string<TSize1, TChar> &str1, const TChar *str2) {
                return !(str1 < str2);
            }

            /// @brief Lexicographical compare between the strings.
            /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_cmp">Reference</a>
            /// @related static_string
            template<std::size_t TSize1, std::size_t TSize2, typename TChar>
            bool operator==(const static_string<TSize1, TChar> &str1, const static_string<TSize2, TChar> &str2) {
                return (str1.size() == str2.size()) && std::equal(str1.begin(), str1.end(), str2.begin());
            }

            /// @brief Equality compare between the strings.
            /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_cmp">Reference</a>
            /// @related static_string
            template<std::size_t TSize1, typename TChar>
            bool operator==(const TChar *str1, const static_string<TSize1, TChar> &str2) {
                return str2 == str1;
            }

            /// @brief Inequality compare between the strings.
            /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_cmp">Reference</a>
            /// @related static_string
            template<std::size_t TSize1, typename TChar>
            bool operator!=(const TChar *str1, const static_string<TSize1, TChar> &str2) {
                return !(str2 == str1);
            }

            /// @brief Inequality compare between the strings.
            /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/operator_cmp">Reference</a>
            /// @related static_string
            template<std::size_t TSize1, std::size_t TSize2, typename TChar>
            bool operator!=(const static_string<TSize1, TChar> &str1, const static_string<TSize2, TChar> &str2) {
                return !(str1 == str2);
            }
        }    // namespace container
    }    // namespace marshalling
}    // namespace nil

namespace std {

    /// @brief Specializes the std::swap algorithm.
    /// @see <a href="http://en.cppreference.com/w/cpp/string/basic_string/swap2">Reference</a>
    /// @related nil::crypto3::marshalling::container::static_string
    template<std::size_t TSize1, std::size_t TSize2, typename TChar>
    void swap(nil::crypto3::marshalling::container::static_string<TSize1, TChar> &str1,
              nil::crypto3::marshalling::container::static_string<TSize2, TChar> &str2) {
        str1.swap(str2);
    }

}    // namespace std
#endif    // MARSHALLING_STATIC_STRING_HPP
