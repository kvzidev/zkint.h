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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <zkint.h>

static void TestUint128(void) {
  zk_uint128_t num_a = zk_uint128_from_uint64(5);
  zk_uint128_t num_b = zk_uint128_from_uint64(10);
  zk_uint128_t num_c = zk_uint128_add(num_a, num_b);
  assert(num_c.limbs[0] == 15);
  assert(num_c.limbs[1] == 0);

  zk_uint128_t max_val;
  max_val.limbs[0] = 0xFFFFFFFFFFFFFFFFULL;
  max_val.limbs[1] = 0xFFFFFFFFFFFFFFFFULL;
  zk_uint128_t one = zk_uint128_from_uint64(1);
  zk_uint128_t zero = zk_uint128_add(max_val, one);
  assert(zero.limbs[0] == 0);
  assert(zero.limbs[1] == 0);

  zk_uint128_t diff = zk_uint128_sub(num_b, num_a);
  assert(diff.limbs[0] == 5);
  assert(diff.limbs[1] == 0);
}

static void TestUint256(void) {
  zk_uint256_t num_a = zk_uint256_from_uint64(0xFFFFFFFFFFFFFFFFULL);
  zk_uint256_t num_b = zk_uint256_from_uint64(2);
  zk_uint256_t num_c = zk_uint256_mul(num_a, num_b);
  assert(num_c.limbs[0] == 0xFFFFFFFFFFFFFFFEULL);
  assert(num_c.limbs[1] == 1);
  assert(num_c.limbs[2] == 0);
  assert(num_c.limbs[3] == 0);
}

static void TestDivMod(void) {
  zk_uint256_t num = zk_uint256_from_uint64(100);
  zk_uint256_t den = zk_uint256_from_uint64(7);
  zk_uint256_t quotient = zk_uint256_div(num, den);
  zk_uint256_t remainder = zk_uint256_mod(num, den);
  assert(quotient.limbs[0] == 14);
  assert(remainder.limbs[0] == 2);
}

static void TestShifts(void) {
  zk_uint128_t num_a = zk_uint128_from_uint64(1);
  zk_uint128_t num_b = zk_uint128_shl(num_a, 64);
  assert(num_b.limbs[0] == 0);
  assert(num_b.limbs[1] == 1);

  zk_uint128_t num_c = zk_uint128_shr(num_b, 64);
  assert(num_c.limbs[0] == 1);
  assert(num_c.limbs[1] == 0);
}

static void TestSigned(void) {
  zk_int128_t num_a = zk_int128_from_int64(-5);
  zk_int128_t num_b = zk_int128_from_int64(2);
  zk_int128_t num_q = zk_int128_div(num_a, num_b);
  zk_int128_t num_r = zk_int128_mod(num_a, num_b);

  assert(num_q.limbs[0] == (uint64_t)-2);
  assert(num_q.limbs[1] == 0xFFFFFFFFFFFFFFFFULL);

  assert(num_r.limbs[0] == (uint64_t)-1);
  assert(num_r.limbs[1] == 0xFFFFFFFFFFFFFFFFULL);
}

static void TestString(void) {
  zk_uint256_t num_a = zk_uint256_from_string("12345678901234567890");
  char buf[128];
  zk_uint256_to_string(num_a, buf, sizeof(buf));
  assert(strcmp(buf, "12345678901234567890") == 0);

  zk_uint256_t num_b = zk_uint256_from_string("0x1234567890ABCDEF");
  zk_uint256_to_string_hex(num_b, buf, sizeof(buf));
  assert(strcmp(buf, "0x1234567890ABCDEF") == 0);
}

static void TestComparison(void) {
  zk_uint128_t num_a = zk_uint128_from_uint64(10);
  zk_uint128_t num_b = zk_uint128_from_uint64(20);
  assert(zk_uint128_compare(num_a, num_b) < 0);
  assert(zk_uint128_compare(num_b, num_a) > 0);
  assert(zk_uint128_compare(num_a, num_a) == 0);
}

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
static void TestGenericMacros(void) {
  zk_uint128_t a128 = zk_uint128_from_uint64(5);
  zk_uint128_t b128 = zk_uint128_from_uint64(10);
  zk_uint128_t c128 = zk_add(a128, b128);
  assert(c128.limbs[0] == 15);

  zk_uint256_t a256 = zk_uint256_from_uint64(50);
  zk_uint256_t b256 = zk_uint256_from_uint64(100);
  zk_uint256_t c256 = zk_add(a256, b256);
  assert(c256.limbs[0] == 150);
}
#endif

int main(void) {
  TestUint128();
  TestUint256();
  TestDivMod();
  TestShifts();
  TestSigned();
  TestString();
  TestComparison();
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
  TestGenericMacros();
#endif
  printf("All C tests passed successfully.\n");
  return 0;
}
