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

#ifndef ZKINT_C_H
#define ZKINT_C_H

#include "zkint_common.h"

/* Helper arithmetic primitives */

ZKINT_INLINE void zk_from_uint64(uint64_t *r, uint64_t val, int n);
ZKINT_INLINE void zk_from_int64(uint64_t *r, int64_t val, int n);

ZKINT_INLINE uint64_t zk_add_with_carry(uint64_t a, uint64_t b,
                                        uint64_t carry_in, uint64_t *sum) {
#if defined(_MSC_VER) && defined(ZKINT_MSVC_64BIT)
  unsigned char carry_out;
  carry_out =
      _addcarry_u64((unsigned char)carry_in, a, b, (unsigned __int64 *)sum);
  return carry_out;
#else
  uint64_t s = a + b + carry_in;
  uint64_t carry_out = (s < a) || (carry_in && (s == a));
  *sum = s;
  return carry_out;
#endif
}

ZKINT_INLINE uint64_t zk_sub_with_borrow(uint64_t a, uint64_t b,
                                         uint64_t borrow_in, uint64_t *diff) {
#if defined(_MSC_VER) && defined(ZKINT_MSVC_64BIT)
  unsigned char borrow_out;
  borrow_out =
      _subborrow_u64((unsigned char)borrow_in, a, b, (unsigned __int64 *)diff);
  return borrow_out;
#else
  uint64_t d = a - b - borrow_in;
  uint64_t borrow_out = (a < b) || (borrow_in && (a == b));
  *diff = d;
  return borrow_out;
#endif
}

ZKINT_INLINE void zk_mul64(uint64_t x, uint64_t y, uint64_t *r_low,
                           uint64_t *r_high) {
#if defined(ZKINT_HAS_INT128)
  unsigned __int128 p = (unsigned __int128)x * y;
  *r_low = (uint64_t)p;
  *r_high = (uint64_t)(p >> 64);
#else
  uint64_t x_lo = (uint32_t)x;
  uint64_t x_hi = x >> 32;
  uint64_t y_lo = (uint32_t)y;
  uint64_t y_hi = y >> 32;

  uint64_t p0 = x_lo * y_lo;
  uint64_t p1 = x_lo * y_hi;
  uint64_t p2 = x_hi * y_lo;
  uint64_t p3 = x_hi * y_hi;

  uint64_t carry = ((p0 >> 32) + (uint32_t)p1 + (uint32_t)p2) >> 32;
  *r_low = p0 + (p1 << 32) + (p2 << 32);
  *r_high = p3 + (p1 >> 32) + (p2 >> 32) + carry;
#endif
}

ZKINT_INLINE uint64_t zk_add_limbs(uint64_t *r, const uint64_t *a,
                                   const uint64_t *b, int n) {
  uint64_t carry = 0;
  for (int i = 0; i < n; ++i) {
    carry = zk_add_with_carry(a[i], b[i], carry, &r[i]);
  }
  return carry;
}

ZKINT_INLINE uint64_t zk_sub_limbs(uint64_t *r, const uint64_t *a,
                                   const uint64_t *b, int n) {
  uint64_t borrow = 0;
  for (int i = 0; i < n; ++i) {
    borrow = zk_sub_with_borrow(a[i], b[i], borrow, &r[i]);
  }
  return borrow;
}

ZKINT_INLINE void zk_mul_limbs(uint64_t *r, const uint64_t *a,
                               const uint64_t *b, int n) {
  uint64_t temp[16] = {0};
  for (int i = 0; i < n; ++i) {
    if (a[i] == 0) { continue; }
    uint64_t carry = 0;
    int limit = n - i;
    for (int j = 0; j < limit; ++j) {
#if defined(ZKINT_HAS_INT128)
      unsigned __int128 sum =
          temp[i + j] + (unsigned __int128)a[i] * b[j] + carry;
      temp[i + j] = (uint64_t)sum;
      carry = (uint64_t)(sum >> 64);
#else
      uint64_t prod_low, prod_high;
      zk_mul64(a[i], b[j], &prod_low, &prod_high);
      uint64_t sum;
      uint64_t carry1 = zk_add_with_carry(temp[i + j], prod_low, 0, &sum);
      uint64_t carry2 = zk_add_with_carry(sum, carry, 0, &temp[i + j]);
      carry = prod_high + carry1 + carry2;
#endif
    }
  }
  for (int i = 0; i < n; ++i) { r[i] = temp[i]; }
}

ZKINT_INLINE int zk_msb_index(const uint64_t *limbs, int n) {
  for (int i = n - 1; i >= 0; --i) {
    if (limbs[i] != 0) {
#if defined(__GNUC__) || defined(__clang__)
      return i * 64 + (63 - __builtin_clzll(limbs[i]));
#elif defined(_MSC_VER) && defined(ZKINT_MSVC_64BIT)
      unsigned long index;
      if (_BitScanReverse64(&index, limbs[i])) { return i * 64 + index; }
#else
      uint64_t val = limbs[i];
      int bit = 63;
      while (bit > 0 && !(val & (1ULL << bit))) { bit--; }
      return i * 64 + bit;
#endif
    }
  }
  return -1;
}

#if !defined(ZKINT_HAS_INT128) &&                     \
    !(defined(_MSC_VER) && defined(ZKINT_MSVC_64BIT))
ZKINT_INLINE uint64_t zk_div128_64_fallback(uint64_t high, uint64_t low,
                                            uint64_t divisor,
                                            uint64_t *remainder) {
  uint64_t q = 0;
  uint64_t r = high;
  for (int i = 63; i >= 0; --i) {
    r = (r << 1) | ((low >> i) & 1);
    if (r >= divisor) {
      r -= divisor;
      q |= (1ULL << i);
    }
  }
  *remainder = r;
  return q;
}
#endif

ZKINT_INLINE uint64_t zk_div128_64(uint64_t high, uint64_t low,
                                   uint64_t divisor, uint64_t *remainder) {
#if defined(ZKINT_HAS_INT128)
  unsigned __int128 dividend = ((unsigned __int128)high << 64) | low;
  *remainder = (uint64_t)(dividend % divisor);
  return (uint64_t)(dividend / divisor);
#elif defined(_MSC_VER) && defined(ZKINT_MSVC_64BIT)
  return _udiv128(high, low, divisor, remainder);
#else
  return zk_div128_64_fallback(high, low, divisor, remainder);
#endif
}

ZKINT_INLINE void zk_divmod_limbs(uint64_t *quot, uint64_t *rem,
                                  const uint64_t *num, const uint64_t *den,
                                  int n) {
  uint64_t q[16] = {0};
  uint64_t r[16] = {0};
  bool is_zero = true;
  for (int i = 0; i < n; ++i) {
    if (den[i] != 0) {
      is_zero = false;
      break;
    }
  }
  if (is_zero) {
    if (quot) {
      for (int i = 0; i < n; ++i) { quot[i] = 0; }
    }
    if (rem) {
      for (int i = 0; i < n; ++i) { rem[i] = 0; }
    }
    return;
  }

  // Fast path: single limb divisor
  bool single_limb = true;
  for (int i = 1; i < n; ++i) {
    if (den[i] != 0) {
      single_limb = false;
      break;
    }
  }
  if (single_limb) {
    uint64_t D = den[0];
    if (D == 0) { return; }
    uint64_t carry = 0;
    for (int i = n - 1; i >= 0; --i) {
      if (carry == 0) {
        if (num[i] < D) {
          q[i] = 0;
          carry = num[i];
        }
        else {
          q[i] = num[i] / D;
          carry = num[i] % D;
        }
      }
      else { q[i] = zk_div128_64(carry, num[i], D, &carry); }
    }
    if (quot) {
      for (int i = 0; i < n; ++i) { quot[i] = q[i]; }
    }
    if (rem) { zk_from_uint64(rem, carry, n); }
    return;
  }

  int msb = zk_msb_index(num, n);
  for (int i = msb; i >= 0; --i) {
    uint64_t carry = 0;
    for (int j = 0; j < n; ++j) {
      uint64_t next_carry = r[j] >> 63;
      r[j] = (r[j] << 1) | carry;
      carry = next_carry;
    }
    int limb_idx = i / 64;
    int bit_idx = i % 64;
    uint64_t bit = (num[limb_idx] >> bit_idx) & 1;
    r[0] |= bit;

    bool r_ge_den = true;
    for (int j = n - 1; j >= 0; --j) {
      if (r[j] < den[j]) {
        r_ge_den = false;
        break;
      }
      else if (r[j] > den[j]) { break; }
    }
    if (r_ge_den) {
      uint64_t borrow = 0;
      for (int j = 0; j < n; ++j) {
        borrow = zk_sub_with_borrow(r[j], den[j], borrow, &r[j]);
      }
      q[limb_idx] |= (1ULL << bit_idx);
    }
  }
  if (quot) {
    for (int i = 0; i < n; ++i) { quot[i] = q[i]; }
  }
  if (rem) {
    for (int i = 0; i < n; ++i) { rem[i] = r[i]; }
  }
}

ZKINT_INLINE void zk_negate_limbs(uint64_t *r, const uint64_t *a, int n) {
  uint64_t carry = 1;
  for (int i = 0; i < n; ++i) {
    carry = zk_add_with_carry(~a[i], 0, carry, &r[i]);
  }
}

ZKINT_INLINE void zk_divmod_limbs_signed(uint64_t *quot, uint64_t *rem,
                                         const uint64_t *num,
                                         const uint64_t *den, int n) {
  uint64_t abs_num[16];
  uint64_t abs_den[16];
  bool num_neg = (num[n - 1] & (1ULL << 63)) != 0;
  bool den_neg = (den[n - 1] & (1ULL << 63)) != 0;
  if (num_neg) { zk_negate_limbs(abs_num, num, n); }
  else {
    for (int i = 0; i < n; ++i) { abs_num[i] = num[i]; }
  }
  if (den_neg) { zk_negate_limbs(abs_den, den, n); }
  else {
    for (int i = 0; i < n; ++i) { abs_den[i] = den[i]; }
  }
  uint64_t q[16];
  uint64_t r[16];
  zk_divmod_limbs(q, r, abs_num, abs_den, n);
  if (quot) {
    if (num_neg != den_neg) { zk_negate_limbs(q, q, n); }
    for (int i = 0; i < n; ++i) { quot[i] = q[i]; }
  }
  if (rem) {
    if (num_neg) { zk_negate_limbs(r, r, n); }
    for (int i = 0; i < n; ++i) { rem[i] = r[i]; }
  }
}

ZKINT_INLINE void zk_shl_limbs(uint64_t *r, const uint64_t *a, int shift,
                               int n) {
  if (shift <= 0) {
    for (int i = 0; i < n; ++i) { r[i] = a[i]; }
    return;
  }
  if (shift >= n * 64) {
    for (int i = 0; i < n; ++i) { r[i] = 0; }
    return;
  }
  int limb_shift = (int)((unsigned int)shift / 64);
  int bit_shift = (int)((unsigned int)shift % 64);
  uint64_t temp[16] = {0};
  if (bit_shift == 0) {
    for (int i = limb_shift; i < n; ++i) { temp[i] = a[i - limb_shift]; }
  }
  else {
    for (int i = n - 1; i >= limb_shift; --i) {
      uint64_t low = a[i - limb_shift];
      uint64_t high = (i - limb_shift - 1 >= 0) ? a[i - limb_shift - 1] : 0;
      temp[i] = (low << bit_shift) | (high >> (64 - bit_shift));
    }
    if (limb_shift < n) { temp[limb_shift] = a[0] << bit_shift; }
  }
  for (int i = 0; i < n; ++i) { r[i] = temp[i]; }
}

ZKINT_INLINE void zk_shr_limbs(uint64_t *r, const uint64_t *a, int shift,
                               int n) {
  if (shift <= 0) {
    for (int i = 0; i < n; ++i) { r[i] = a[i]; }
    return;
  }
  if (shift >= n * 64) {
    for (int i = 0; i < n; ++i) { r[i] = 0; }
    return;
  }
  int limb_shift = (int)((unsigned int)shift / 64);
  int bit_shift = (int)((unsigned int)shift % 64);
  uint64_t temp[16] = {0};
  if (bit_shift == 0) {
    for (int i = 0; i < n - limb_shift; ++i) { temp[i] = a[i + limb_shift]; }
  }
  else {
    for (int i = 0; i < n - limb_shift; ++i) {
      uint64_t low = a[i + limb_shift];
      uint64_t high = (i + limb_shift + 1 < n) ? a[i + limb_shift + 1] : 0;
      temp[i] = (low >> bit_shift) | (high << (64 - bit_shift));
    }
  }
  for (int i = 0; i < n; ++i) { r[i] = temp[i]; }
}

ZKINT_INLINE void zk_sar_limbs(uint64_t *r, const uint64_t *a, int shift,
                               int n) {
  if (shift <= 0) {
    for (int i = 0; i < n; ++i) { r[i] = a[i]; }
    return;
  }
  bool is_negative = (a[n - 1] & (1ULL << 63)) != 0;
  if (shift >= n * 64) {
    uint64_t fill = is_negative ? 0xFFFFFFFFFFFFFFFFULL : 0;
    for (int i = 0; i < n; ++i) { r[i] = fill; }
    return;
  }
  int limb_shift = (int)((unsigned int)shift / 64);
  int bit_shift = (int)((unsigned int)shift % 64);
  uint64_t temp[16];
  uint64_t fill = is_negative ? 0xFFFFFFFFFFFFFFFFULL : 0;
  if (bit_shift == 0) {
    for (int i = 0; i < n; ++i) {
      temp[i] = (i + limb_shift < n) ? a[i + limb_shift] : fill;
    }
  }
  else {
    for (int i = 0; i < n; ++i) {
      if (i + limb_shift < n) {
        uint64_t low = a[i + limb_shift];
        uint64_t high = (i + limb_shift + 1 < n) ? a[i + limb_shift + 1] : fill;
        temp[i] = (low >> bit_shift) | (high << (64 - bit_shift));
      }
      else { temp[i] = fill; }
    }
  }
  for (int i = 0; i < n; ++i) { r[i] = temp[i]; }
}

ZKINT_INLINE int zk_compare_limbs_unsigned(const uint64_t *a, const uint64_t *b,
                                           int n) {
  for (int i = n - 1; i >= 0; --i) {
    if (a[i] < b[i]) { return -1; }
    if (a[i] > b[i]) { return 1; }
  }
  return 0;
}

ZKINT_INLINE int zk_compare_limbs_signed(const uint64_t *a, const uint64_t *b,
                                         int n) {
  bool a_neg = (a[n - 1] & (1ULL << 63)) != 0;
  bool b_neg = (b[n - 1] & (1ULL << 63)) != 0;
  if (a_neg && !b_neg) { return -1; }
  if (!a_neg && b_neg) { return 1; }
  return zk_compare_limbs_unsigned(a, b, n);
}

ZKINT_CONSTEXPR_FN ZKINT_INLINE void zk_from_uint64(uint64_t *r, uint64_t val,
                                                    int n) {
  r[0] = val;
  for (int i = 1; i < n; ++i) { r[i] = 0; }
}

ZKINT_CONSTEXPR_FN ZKINT_INLINE void zk_from_int64(uint64_t *r, int64_t val,
                                                   int n) {
  r[0] = (uint64_t)val;
  uint64_t fill = (val < 0) ? 0xFFFFFFFFFFFFFFFFULL : 0;
  for (int i = 1; i < n; ++i) { r[i] = fill; }
}

ZKINT_INLINE void zk_from_string_limbs(uint64_t *r, const char *str, int n) {
  for (int i = 0; i < n; ++i) { r[i] = 0; }
  if (!str || *str == '\0') { return; }
  while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') { str++; }
  bool neg = false;
  if (*str == '-') {
    neg = true;
    str++;
  }
  else if (*str == '+') { str++; }
  bool hex = false;
  if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
    hex = true;
    str += 2;
  }
  if (hex) {
    while (*str) {
      char c = *str++;
      int digit = 0;
      if (c >= '0' && c <= '9') { digit = c - '0'; }
      else if (c >= 'a' && c <= 'f') { digit = c - 'a' + 10; }
      else if (c >= 'A' && c <= 'F') { digit = c - 'A' + 10; }
      else { break; }
      uint64_t carry = digit;
      for (int i = 0; i < n; ++i) {
        uint64_t next_carry = r[i] >> 60;
        r[i] = (r[i] << 4) | carry;
        carry = next_carry;
      }
    }
  }
  else {
    while (*str) {
      char c = *str++;
      if (c < '0' || c > '9') { break; }
      int digit = c - '0';
      uint64_t carry = digit;
      for (int i = 0; i < n; ++i) {
#if defined(ZKINT_HAS_INT128)
        unsigned __int128 p = (unsigned __int128)r[i] * 10 + carry;
        r[i] = (uint64_t)p;
        carry = (uint64_t)(p >> 64);
#else
        uint64_t prod_low, prod_high;
        zk_mul64(r[i], 10, &prod_low, &prod_high);
        uint64_t sum;
        uint64_t c1 = zk_add_with_carry(prod_low, carry, 0, &sum);
        r[i] = sum;
        carry = prod_high + c1;
#endif
      }
    }
  }
  if (neg) { zk_negate_limbs(r, r, n); }
}

ZKINT_INLINE void zk_to_string_limbs(const uint64_t *a, char *buf, int buf_len,
                                     bool signed_type, bool hex, int n) {
  if (buf_len <= 0) { return; }
  buf[0] = '\0';
  uint64_t temp[16];
  for (int i = 0; i < n; ++i) { temp[i] = a[i]; }
  bool neg = false;
  if (signed_type && (temp[n - 1] & (1ULL << 63)) != 0) {
    neg = true;
    zk_negate_limbs(temp, temp, n);
  }
  bool is_zero = true;
  for (int i = 0; i < n; ++i) {
    if (temp[i] != 0) {
      is_zero = false;
      break;
    }
  }
  if (is_zero) {
    if (buf_len > 1) {
      buf[0] = '0';
      buf[1] = '\0';
    }
    return;
  }
  char rev[2048];
  int idx = 0;
  if (hex) {
    while (true) {
      bool tz = true;
      for (int i = 0; i < n; ++i) {
        if (temp[i] != 0) {
          tz = false;
          break;
        }
      }
      if (tz) { break; }
      int digit = temp[0] & 0xF;
      rev[idx++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
      uint64_t carry = 0;
      for (int i = n - 1; i >= 0; --i) {
        uint64_t next_carry = temp[i] & 0xF;
        temp[i] = (temp[i] >> 4) | (carry << 60);
        carry = next_carry;
      }
    }
    rev[idx++] = 'x';
    rev[idx++] = '0';
  }
  else {
    while (true) {
      bool tz = true;
      for (int i = 0; i < n; ++i) {
        if (temp[i] != 0) {
          tz = false;
          break;
        }
      }
      if (tz) { break; }
      uint64_t quot[16];
      uint64_t rem[16];
      uint64_t ten[16] = {0};
      ten[0] = 10;
      zk_divmod_limbs(quot, rem, temp, ten, n);
      rev[idx++] = '0' + (int)rem[0];
      for (int i = 0; i < n; ++i) { temp[i] = quot[i]; }
    }
  }
  if (neg) { rev[idx++] = '-'; }
  int b_idx = 0;
  for (int i = idx - 1; i >= 0 && b_idx < buf_len - 1; --i) {
    buf[b_idx++] = rev[i];
  }
  buf[b_idx] = '\0';
}

/* Struct and function generator */

#define DEFINE_ZKINT_TYPE(bits, n) \
  typedef struct {                 \
    uint64_t limbs[n];             \
  } zk_uint##bits##_t;             \
  typedef struct {                 \
    uint64_t limbs[n];             \
  } zk_int##bits##_t;

DEFINE_ZKINT_TYPE(128, 2)
DEFINE_ZKINT_TYPE(256, 4)
DEFINE_ZKINT_TYPE(512, 8)
DEFINE_ZKINT_TYPE(1024, 16)

#define DEFINE_ZKINT_FUNCTIONS(bits, suffix, n, is_signed)                    \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_add(zk_##suffix##_t a,           \
                                                 zk_##suffix##_t b) {         \
    zk_##suffix##_t r;                                                        \
    zk_add_limbs(r.limbs, a.limbs, b.limbs, n);                               \
    return r;                                                                 \
  }                                                                           \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_sub(zk_##suffix##_t a,           \
                                                 zk_##suffix##_t b) {         \
    zk_##suffix##_t r;                                                        \
    zk_sub_limbs(r.limbs, a.limbs, b.limbs, n);                               \
    return r;                                                                 \
  }                                                                           \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_mul(zk_##suffix##_t a,           \
                                                 zk_##suffix##_t b) {         \
    zk_##suffix##_t r;                                                        \
    zk_mul_limbs(r.limbs, a.limbs, b.limbs, n);                               \
    return r;                                                                 \
  }                                                                           \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_div(zk_##suffix##_t a,           \
                                                 zk_##suffix##_t b) {         \
    zk_##suffix##_t q;                                                        \
    if (is_signed) {                                                          \
      zk_divmod_limbs_signed(q.limbs, NULL, a.limbs, b.limbs, n);             \
    }                                                                         \
    else { zk_divmod_limbs(q.limbs, NULL, a.limbs, b.limbs, n); }             \
    return q;                                                                 \
  }                                                                           \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_mod(zk_##suffix##_t a,           \
                                                 zk_##suffix##_t b) {         \
    zk_##suffix##_t r;                                                        \
    if (is_signed) {                                                          \
      zk_divmod_limbs_signed(NULL, r.limbs, a.limbs, b.limbs, n);             \
    }                                                                         \
    else { zk_divmod_limbs(NULL, r.limbs, a.limbs, b.limbs, n); }             \
    return r;                                                                 \
  }                                                                           \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_and(zk_##suffix##_t a,           \
                                                 zk_##suffix##_t b) {         \
    zk_##suffix##_t r;                                                        \
    for (int i = 0; i < n; ++i) r.limbs[i] = a.limbs[i] & b.limbs[i];         \
    return r;                                                                 \
  }                                                                           \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_or(zk_##suffix##_t a,            \
                                                zk_##suffix##_t b) {          \
    zk_##suffix##_t r;                                                        \
    for (int i = 0; i < n; ++i) r.limbs[i] = a.limbs[i] | b.limbs[i];         \
    return r;                                                                 \
  }                                                                           \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_xor(zk_##suffix##_t a,           \
                                                 zk_##suffix##_t b) {         \
    zk_##suffix##_t r;                                                        \
    for (int i = 0; i < n; ++i) r.limbs[i] = a.limbs[i] ^ b.limbs[i];         \
    return r;                                                                 \
  }                                                                           \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_not(zk_##suffix##_t a) {         \
    zk_##suffix##_t r;                                                        \
    for (int i = 0; i < n; ++i) r.limbs[i] = ~a.limbs[i];                     \
    return r;                                                                 \
  }                                                                           \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_shl(zk_##suffix##_t a,           \
                                                 int shift) {                 \
    zk_##suffix##_t r;                                                        \
    zk_shl_limbs(r.limbs, a.limbs, shift, n);                                 \
    return r;                                                                 \
  }                                                                           \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_shr(zk_##suffix##_t a,           \
                                                 int shift) {                 \
    zk_##suffix##_t r;                                                        \
    if (is_signed) { zk_sar_limbs(r.limbs, a.limbs, shift, n); }              \
    else { zk_shr_limbs(r.limbs, a.limbs, shift, n); }                        \
    return r;                                                                 \
  }                                                                           \
  ZKINT_INLINE int zk_##suffix##_compare(zk_##suffix##_t a,                   \
                                         zk_##suffix##_t b) {                 \
    if (is_signed) { return zk_compare_limbs_signed(a.limbs, b.limbs, n); }   \
    else { return zk_compare_limbs_unsigned(a.limbs, b.limbs, n); }           \
  }                                                                           \
  ZKINT_INLINE bool zk_##suffix##_eq(zk_##suffix##_t a, zk_##suffix##_t b) {  \
    return zk_##suffix##_compare(a, b) == 0;                                  \
  }                                                                           \
  ZKINT_INLINE bool zk_##suffix##_lt(zk_##suffix##_t a, zk_##suffix##_t b) {  \
    return zk_##suffix##_compare(a, b) < 0;                                   \
  }                                                                           \
  ZKINT_INLINE bool zk_##suffix##_gt(zk_##suffix##_t a, zk_##suffix##_t b) {  \
    return zk_##suffix##_compare(a, b) > 0;                                   \
  }                                                                           \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_from_uint64(uint64_t val) {      \
    zk_##suffix##_t r;                                                        \
    zk_from_uint64(r.limbs, val, n);                                          \
    return r;                                                                 \
  }                                                                           \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_from_int64(int64_t val) {        \
    zk_##suffix##_t r;                                                        \
    zk_from_int64(r.limbs, val, n);                                           \
    return r;                                                                 \
  }                                                                           \
  ZKINT_INLINE zk_##suffix##_t zk_##suffix##_from_string(const char *str) {   \
    zk_##suffix##_t r;                                                        \
    zk_from_string_limbs(r.limbs, str, n);                                    \
    return r;                                                                 \
  }                                                                           \
  ZKINT_INLINE void zk_##suffix##_to_string(zk_##suffix##_t a, char *buf,     \
                                            int buf_len) {                    \
    zk_to_string_limbs(a.limbs, buf, buf_len, is_signed, false, n);           \
  }                                                                           \
  ZKINT_INLINE void zk_##suffix##_to_string_hex(zk_##suffix##_t a, char *buf, \
                                                int buf_len) {                \
    zk_to_string_limbs(a.limbs, buf, buf_len, is_signed, true, n);            \
  }

DEFINE_ZKINT_FUNCTIONS(128, uint128, 2, false)
DEFINE_ZKINT_FUNCTIONS(128, int128, 2, true)
DEFINE_ZKINT_FUNCTIONS(256, uint256, 4, false)
DEFINE_ZKINT_FUNCTIONS(256, int256, 4, true)
DEFINE_ZKINT_FUNCTIONS(512, uint512, 8, false)
DEFINE_ZKINT_FUNCTIONS(512, int512, 8, true)
DEFINE_ZKINT_FUNCTIONS(1024, uint1024, 16, false)
DEFINE_ZKINT_FUNCTIONS(1024, int1024, 16, true)

/* C11 _Generic overloads */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define zk_add(a, b)                      \
  _Generic((a),                           \
      zk_uint128_t: zk_uint128_add,       \
      zk_int128_t: zk_int128_add,         \
      zk_uint256_t: zk_uint256_add,       \
      zk_int256_t: zk_int256_add,         \
      zk_uint512_t: zk_uint512_add,       \
      zk_int512_t: zk_int512_add,         \
      zk_uint1024_t: zk_uint1024_add,     \
      zk_int1024_t: zk_int1024_add)(a, b)
#define zk_sub(a, b)                      \
  _Generic((a),                           \
      zk_uint128_t: zk_uint128_sub,       \
      zk_int128_t: zk_int128_sub,         \
      zk_uint256_t: zk_uint256_sub,       \
      zk_int256_t: zk_int256_sub,         \
      zk_uint512_t: zk_uint512_sub,       \
      zk_int512_t: zk_int512_sub,         \
      zk_uint1024_t: zk_uint1024_sub,     \
      zk_int1024_t: zk_int1024_sub)(a, b)
#define zk_mul(a, b)                      \
  _Generic((a),                           \
      zk_uint128_t: zk_uint128_mul,       \
      zk_int128_t: zk_int128_mul,         \
      zk_uint256_t: zk_uint256_mul,       \
      zk_int256_t: zk_int256_mul,         \
      zk_uint512_t: zk_uint512_mul,       \
      zk_int512_t: zk_int512_mul,         \
      zk_uint1024_t: zk_uint1024_mul,     \
      zk_int1024_t: zk_int1024_mul)(a, b)
#define zk_div(a, b)                      \
  _Generic((a),                           \
      zk_uint128_t: zk_uint128_div,       \
      zk_int128_t: zk_int128_div,         \
      zk_uint256_t: zk_uint256_div,       \
      zk_int256_t: zk_int256_div,         \
      zk_uint512_t: zk_uint512_div,       \
      zk_int512_t: zk_int512_div,         \
      zk_uint1024_t: zk_uint1024_div,     \
      zk_int1024_t: zk_int1024_div)(a, b)
#define zk_mod(a, b)                      \
  _Generic((a),                           \
      zk_uint128_t: zk_uint128_mod,       \
      zk_int128_t: zk_int128_mod,         \
      zk_uint256_t: zk_uint256_mod,       \
      zk_int256_t: zk_int256_mod,         \
      zk_uint512_t: zk_uint512_mod,       \
      zk_int512_t: zk_int512_mod,         \
      zk_uint1024_t: zk_uint1024_mod,     \
      zk_int1024_t: zk_int1024_mod)(a, b)
#define zk_and(a, b)                      \
  _Generic((a),                           \
      zk_uint128_t: zk_uint128_and,       \
      zk_int128_t: zk_int128_and,         \
      zk_uint256_t: zk_uint256_and,       \
      zk_int256_t: zk_int256_and,         \
      zk_uint512_t: zk_uint512_and,       \
      zk_int512_t: zk_int512_and,         \
      zk_uint1024_t: zk_uint1024_and,     \
      zk_int1024_t: zk_int1024_and)(a, b)
#define zk_or(a, b)                      \
  _Generic((a),                          \
      zk_uint128_t: zk_uint128_or,       \
      zk_int128_t: zk_int128_or,         \
      zk_uint256_t: zk_uint256_or,       \
      zk_int256_t: zk_int256_or,         \
      zk_uint512_t: zk_uint512_or,       \
      zk_int512_t: zk_int512_or,         \
      zk_uint1024_t: zk_uint1024_or,     \
      zk_int1024_t: zk_int1024_or)(a, b)
#define zk_xor(a, b)                      \
  _Generic((a),                           \
      zk_uint128_t: zk_uint128_xor,       \
      zk_int128_t: zk_int128_xor,         \
      zk_uint256_t: zk_uint256_xor,       \
      zk_int256_t: zk_int256_xor,         \
      zk_uint512_t: zk_uint512_xor,       \
      zk_int512_t: zk_int512_xor,         \
      zk_uint1024_t: zk_uint1024_xor,     \
      zk_int1024_t: zk_int1024_xor)(a, b)
#define zk_not(a)                      \
  _Generic((a),                        \
      zk_uint128_t: zk_uint128_not,    \
      zk_int128_t: zk_int128_not,      \
      zk_uint256_t: zk_uint256_not,    \
      zk_int256_t: zk_int256_not,      \
      zk_uint512_t: zk_uint512_not,    \
      zk_int512_t: zk_int512_not,      \
      zk_uint1024_t: zk_uint1024_not,  \
      zk_int1024_t: zk_int1024_not)(a)
#define zk_shl(a, shift)                      \
  _Generic((a),                               \
      zk_uint128_t: zk_uint128_shl,           \
      zk_int128_t: zk_int128_shl,             \
      zk_uint256_t: zk_uint256_shl,           \
      zk_int256_t: zk_int256_shl,             \
      zk_uint512_t: zk_uint512_shl,           \
      zk_int512_t: zk_int512_shl,             \
      zk_uint1024_t: zk_uint1024_shl,         \
      zk_int1024_t: zk_int1024_shl)(a, shift)
#define zk_shr(a, shift)                      \
  _Generic((a),                               \
      zk_uint128_t: zk_uint128_shr,           \
      zk_int128_t: zk_int128_shr,             \
      zk_uint256_t: zk_uint256_shr,           \
      zk_int256_t: zk_int256_shr,             \
      zk_uint512_t: zk_uint512_shr,           \
      zk_int512_t: zk_int512_shr,             \
      zk_uint1024_t: zk_uint1024_shr,         \
      zk_int1024_t: zk_int1024_shr)(a, shift)
#define zk_eq(a, b)                      \
  _Generic((a),                          \
      zk_uint128_t: zk_uint128_eq,       \
      zk_int128_t: zk_int128_eq,         \
      zk_uint256_t: zk_uint256_eq,       \
      zk_int256_t: zk_int256_eq,         \
      zk_uint512_t: zk_uint512_eq,       \
      zk_int512_t: zk_int512_eq,         \
      zk_uint1024_t: zk_uint1024_eq,     \
      zk_int1024_t: zk_int1024_eq)(a, b)
#define zk_lt(a, b)                      \
  _Generic((a),                          \
      zk_uint128_t: zk_uint128_lt,       \
      zk_int128_t: zk_int128_lt,         \
      zk_uint256_t: zk_uint256_lt,       \
      zk_int256_t: zk_int256_lt,         \
      zk_uint512_t: zk_uint512_lt,       \
      zk_int512_t: zk_int512_lt,         \
      zk_uint1024_t: zk_uint1024_lt,     \
      zk_int1024_t: zk_int1024_lt)(a, b)
#define zk_gt(a, b)                      \
  _Generic((a),                          \
      zk_uint128_t: zk_uint128_gt,       \
      zk_int128_t: zk_int128_gt,         \
      zk_uint256_t: zk_uint256_gt,       \
      zk_int256_t: zk_int256_gt,         \
      zk_uint512_t: zk_uint512_gt,       \
      zk_int512_t: zk_int512_gt,         \
      zk_uint1024_t: zk_uint1024_gt,     \
      zk_int1024_t: zk_int1024_gt)(a, b)
#endif

#endif /* ZKINT_C_H */
