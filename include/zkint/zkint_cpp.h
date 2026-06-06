/*
 * Copyright 2026 Kevin Zanzi
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ZKINT_CPP_H
#define ZKINT_CPP_H

#include <iostream>
#include <string>
#include <type_traits>

#include "zkint_c.h"

namespace zk {

template<size_t Bits, bool Signed>
class bigint;

template<typename T>
struct is_bigint: std::false_type {};

template<size_t Bits, bool Signed>
struct is_bigint<bigint<Bits, Signed>>: std::true_type {};

template<size_t...>
struct seq {};
template<size_t N, size_t... Is>
struct gen_seq: gen_seq<N - 1, N - 1, Is...> {};
template<size_t... Is>
struct gen_seq<0, Is...> {
  using type = seq<Is...>;
};

template<size_t Bits, bool Signed>
struct bigint_c_type;

template<>
struct bigint_c_type<128, false> {
  using type = zk_uint128_t;
};
template<>
struct bigint_c_type<128, true> {
  using type = zk_int128_t;
};
template<>
struct bigint_c_type<256, false> {
  using type = zk_uint256_t;
};
template<>
struct bigint_c_type<256, true> {
  using type = zk_int256_t;
};
template<>
struct bigint_c_type<512, false> {
  using type = zk_uint512_t;
};
template<>
struct bigint_c_type<512, true> {
  using type = zk_int512_t;
};
template<>
struct bigint_c_type<1024, false> {
  using type = zk_uint1024_t;
};
template<>
struct bigint_c_type<1024, true> {
  using type = zk_int1024_t;
};

template<size_t Bits, bool Signed>
class bigint {
 public:
  using c_type = typename bigint_c_type<Bits, Signed>::type;

  static_assert(Bits % 64 == 0, "Bits must be a multiple of 64");
  static_assert(Bits >= 128 && Bits <= 1024,
                "Bits must be between 128 and 1024");
  static constexpr size_t LimbsCount = Bits / 64;

  uint64_t limbs[LimbsCount];

  ZKINT_CONSTEXPR bigint() noexcept: limbs {} {}

 private:
  template<typename T, size_t... Is>
  ZKINT_CONSTEXPR bigint(T val, seq<Is...>) noexcept
      : limbs {(Is == 0
                    ? static_cast<uint64_t>(val)
                    : (std::is_signed<T>::value && static_cast<int64_t>(val) < 0
                           ? 0xFFFFFFFFFFFFFFFFULL
                           : 0))...} {}

  template<size_t OtherBits, bool OtherSigned, size_t... Is>
  ZKINT_CONSTEXPR bigint(const bigint<OtherBits, OtherSigned> &other,
                         seq<Is...>) noexcept
      : limbs {(Is < (OtherBits / 64)
                    ? other.limbs[Is]
                    : (OtherSigned && (other.limbs[(OtherBits / 64) - 1] &
                                       (1ULL << 63))
                           ? 0xFFFFFFFFFFFFFFFFULL
                           : 0))...} {}

 public:
  template<typename T,
           typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  ZKINT_CONSTEXPR bigint(T val) noexcept
      : bigint(val, typename gen_seq<LimbsCount>::type {}) {}

  explicit bigint(const char *str) {
    zk_from_string_limbs(limbs, str, LimbsCount);
  }

  explicit bigint(const std::string &str) {
    zk_from_string_limbs(limbs, str.c_str(), LimbsCount);
  }

  bigint(const c_type &c) noexcept {
    const uint64_t *src = reinterpret_cast<const uint64_t *>(&c);
    for (size_t i = 0; i < LimbsCount; ++i) { limbs[i] = src[i]; }
  }

  operator c_type() const noexcept {
    c_type c;
    uint64_t *dest = reinterpret_cast<uint64_t *>(&c);
    for (size_t i = 0; i < LimbsCount; ++i) { dest[i] = limbs[i]; }
    return c;
  }

  template<
      size_t OtherBits, bool OtherSigned,
      typename std::enable_if<(OtherBits < Bits) ||
                                  (OtherBits == Bits && OtherSigned && !Signed),
                              int>::type = 0>
  ZKINT_CONSTEXPR bigint(const bigint<OtherBits, OtherSigned> &other) noexcept
      : bigint(other, typename gen_seq<LimbsCount>::type {}) {}

  explicit operator bool() const noexcept {
    for (size_t i = 0; i < LimbsCount; ++i) {
      if (limbs[i] != 0) { return true; }
    }
    return false;
  }

  explicit operator uint64_t() const noexcept { return limbs[0]; }
  explicit operator int64_t() const noexcept {
    return static_cast<int64_t>(limbs[0]);
  }

  std::string to_string(bool hex = false) const {
    char buf[2048];
    zk_to_string_limbs(limbs, buf, sizeof(buf), Signed, hex, LimbsCount);
    return std::string(buf);
  }

  bigint operator~() const noexcept {
    bigint r;
    for (size_t i = 0; i < LimbsCount; ++i) { r.limbs[i] = ~limbs[i]; }
    return r;
  }

  bigint operator-() const noexcept {
    bigint r;
    zk_negate_limbs(r.limbs, limbs, LimbsCount);
    return r;
  }

  bigint operator+() const noexcept { return *this; }

  bigint &operator++() noexcept {
    uint64_t carry = 1;
    for (size_t i = 0; i < LimbsCount; ++i) {
      carry = zk_add_with_carry(limbs[i], 0, carry, &limbs[i]);
    }
    return *this;
  }

  bigint operator++(int) noexcept {
    bigint tmp(*this);
    ++(*this);
    return tmp;
  }

  bigint &operator--() noexcept {
    uint64_t borrow = 1;
    for (size_t i = 0; i < LimbsCount; ++i) {
      borrow = zk_sub_with_borrow(limbs[i], 0, borrow, &limbs[i]);
    }
    return *this;
  }

  bigint operator--(int) noexcept {
    bigint tmp(*this);
    --(*this);
    return tmp;
  }

  bigint &operator+=(const bigint &rhs) noexcept {
    zk_add_limbs(limbs, limbs, rhs.limbs, LimbsCount);
    return *this;
  }

  bigint &operator-=(const bigint &rhs) noexcept {
    zk_sub_limbs(limbs, limbs, rhs.limbs, LimbsCount);
    return *this;
  }

  bigint &operator*=(const bigint &rhs) noexcept {
    zk_mul_limbs(limbs, limbs, rhs.limbs, LimbsCount);
    return *this;
  }

  bigint &operator/=(const bigint &rhs) noexcept {
    if (Signed) {
      zk_divmod_limbs_signed(limbs, nullptr, limbs, rhs.limbs, LimbsCount);
    }
    else { zk_divmod_limbs(limbs, nullptr, limbs, rhs.limbs, LimbsCount); }
    return *this;
  }

  bigint &operator%=(const bigint &rhs) noexcept {
    if (Signed) {
      zk_divmod_limbs_signed(nullptr, limbs, limbs, rhs.limbs, LimbsCount);
    }
    else { zk_divmod_limbs(nullptr, limbs, limbs, rhs.limbs, LimbsCount); }
    return *this;
  }

  bigint &operator&=(const bigint &rhs) noexcept {
    for (size_t i = 0; i < LimbsCount; ++i) { limbs[i] &= rhs.limbs[i]; }
    return *this;
  }

  bigint &operator|=(const bigint &rhs) noexcept {
    for (size_t i = 0; i < LimbsCount; ++i) { limbs[i] |= rhs.limbs[i]; }
    return *this;
  }

  bigint &operator^=(const bigint &rhs) noexcept {
    for (size_t i = 0; i < LimbsCount; ++i) { limbs[i] ^= rhs.limbs[i]; }
    return *this;
  }

  bigint &operator<<=(int shift) noexcept {
    zk_shl_limbs(limbs, limbs, shift, LimbsCount);
    return *this;
  }

  bigint &operator>>=(int shift) noexcept {
    if (Signed) { zk_sar_limbs(limbs, limbs, shift, LimbsCount); }
    else { zk_shr_limbs(limbs, limbs, shift, LimbsCount); }
    return *this;
  }

  int compare(const bigint &other) const noexcept {
    if (Signed) {
      return zk_compare_limbs_signed(limbs, other.limbs, LimbsCount);
    }
    else { return zk_compare_limbs_unsigned(limbs, other.limbs, LimbsCount); }
  }

  friend bigint operator+(const bigint &lhs, const bigint &rhs) noexcept {
    bigint r(lhs);
    r += rhs;
    return r;
  }
  friend bigint operator-(const bigint &lhs, const bigint &rhs) noexcept {
    bigint r(lhs);
    r -= rhs;
    return r;
  }
  friend bigint operator*(const bigint &lhs, const bigint &rhs) noexcept {
    bigint r(lhs);
    r *= rhs;
    return r;
  }
  friend bigint operator/(const bigint &lhs, const bigint &rhs) noexcept {
    bigint r(lhs);
    r /= rhs;
    return r;
  }
  friend bigint operator%(const bigint &lhs, const bigint &rhs) noexcept {
    bigint r(lhs);
    r %= rhs;
    return r;
  }
  friend bigint operator&(const bigint &lhs, const bigint &rhs) noexcept {
    bigint r(lhs);
    r &= rhs;
    return r;
  }
  friend bigint operator|(const bigint &lhs, const bigint &rhs) noexcept {
    bigint r(lhs);
    r |= rhs;
    return r;
  }
  friend bigint operator^(const bigint &lhs, const bigint &rhs) noexcept {
    bigint r(lhs);
    r ^= rhs;
    return r;
  }
  friend bigint operator<<(const bigint &lhs, int shift) noexcept {
    bigint r(lhs);
    r <<= shift;
    return r;
  }
  friend bigint operator>>(const bigint &lhs, int shift) noexcept {
    bigint r(lhs);
    r >>= shift;
    return r;
  }

  friend bool operator==(const bigint &lhs, const bigint &rhs) noexcept {
    return lhs.compare(rhs) == 0;
  }
  friend bool operator!=(const bigint &lhs, const bigint &rhs) noexcept {
    return lhs.compare(rhs) != 0;
  }
  friend bool operator<(const bigint &lhs, const bigint &rhs) noexcept {
    return lhs.compare(rhs) < 0;
  }
  friend bool operator>(const bigint &lhs, const bigint &rhs) noexcept {
    return lhs.compare(rhs) > 0;
  }
  friend bool operator<=(const bigint &lhs, const bigint &rhs) noexcept {
    return lhs.compare(rhs) <= 0;
  }
  friend bool operator>=(const bigint &lhs, const bigint &rhs) noexcept {
    return lhs.compare(rhs) >= 0;
  }

  friend std::ostream &operator<<(std::ostream &os, const bigint &val) {
    os << val.to_string();
    return os;
  }
};

using uint128_t = bigint<128, false>;
using int128_t = bigint<128, true>;
using uint256_t = bigint<256, false>;
using int256_t = bigint<256, true>;
using uint512_t = bigint<512, false>;
using int512_t = bigint<512, true>;
using uint1024_t = bigint<1024, false>;
using int1024_t = bigint<1024, true>;

}  // namespace zk

#endif /* ZKINT_CPP_H */
