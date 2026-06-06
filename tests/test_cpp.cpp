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

#include <zkint.h>

#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>

void TestBasicArithmetic() {
  zk::uint128_t num_a = 5;
  zk::uint128_t num_b = 10;
  zk::uint128_t num_c = num_a + num_b;
  assert(num_c == 15);

  zk::uint128_t num_d = num_b - num_a;
  assert(num_d == 5);

  zk::uint128_t num_e = num_a * num_b;
  assert(num_e == 50);

  zk::uint128_t num_f = num_b / num_a;
  assert(num_f == 2);

  zk::uint128_t num_g = num_b % num_a;
  assert(num_g == 0);
}

void TestComparisons() {
  zk::uint128_t num_a = 5;
  zk::uint128_t num_b = 10;

  assert(num_a < num_b);
  assert(num_b > num_a);
  assert(num_a <= num_b);
  assert(num_b >= num_a);
  assert(num_a != num_b);
  assert(num_a == 5);
}

void TestBitwise() {
  zk::uint128_t num_a = 0xF0F0F0F0;
  zk::uint128_t num_b = 0x0F0F0F0F;

  assert((num_a & num_b) == 0);
  assert((num_a | num_b) == 0xFFFFFFFF);
  assert((num_a ^ num_b) == 0xFFFFFFFF);

  zk::uint128_t num_c = 1;
  assert((num_c << 64) == zk::uint128_t("0x10000000000000000"));
  assert(((num_c << 64) >> 64) == 1);
}

void TestTypePromotion() {
  zk::uint128_t num_a = 5;
  zk::uint256_t num_b = 10;

  // uint128_t + uint256_t should promote to uint256_t
  auto num_c = num_a + num_b;
  static_assert(std::is_same<decltype(num_c), zk::uint256_t>::value,
                "Promotion failed!");
  assert(num_c == 15);

  // uint256_t + int should promote to uint256_t
  auto num_d = num_b + 5;
  static_assert(std::is_same<decltype(num_d), zk::uint256_t>::value,
                "Promotion failed!");
  assert(num_d == 15);

  // int + uint256_t should promote to uint256_t
  auto num_e = 5 + num_b;
  static_assert(std::is_same<decltype(num_e), zk::uint256_t>::value,
                "Promotion failed!");
  assert(num_e == 15);

  // int128_t + uint128_t should promote to uint128_t (signed + unsigned ->
  // unsigned)
  zk::int128_t num_sa = 5;
  zk::uint128_t num_ua = 10;
  auto num_f = num_sa + num_ua;
  static_assert(std::is_same<decltype(num_f), zk::uint128_t>::value,
                "Promotion failed!");
  assert(num_f == 15);
}

void TestIncDec() {
  zk::uint128_t num_a = 5;
  assert(++num_a == 6);
  assert(num_a++ == 6);
  assert(num_a == 7);
  assert(--num_a == 6);
  assert(num_a-- == 6);
  assert(num_a == 5);
}

void TestString() {
  zk::uint256_t num_a("123456789012345678901234567890");
  assert(num_a.to_string() == "123456789012345678901234567890");
}

void TestEdgeCases() {
  // 1. Division by zero
  zk::uint128_t num_a = 5;
  zk::uint128_t num_b = 0;
  zk::uint128_t num_q = num_a / num_b;
  zk::uint128_t num_r = num_a % num_b;
  assert(num_q == 0);
  assert(num_r == 0);

  // 2. Arithmetic shift right sign propagation
  zk::int128_t neg_one = -1;
  assert((neg_one >> 1) == -1);
  assert((neg_one >> 64) == -1);
  assert((neg_one >> 128) == -1);

  // 3. Multiplication of max values (modulo arithmetic)
  zk::uint128_t max_u128 = ~zk::uint128_t(0);
  assert(max_u128 * max_u128 == 1);

  // 4. Large shifts
  zk::uint128_t one = 1;
  assert((one << 127) != 0);
  assert((one << 128) == 0);
  assert((one << 200) == 0);

  // 5. Casting to bool
  zk::uint128_t high_bit = zk::uint128_t(1) << 127;
  assert(static_cast<bool>(high_bit) == true);
  assert(static_cast<bool>(high_bit >> 127) == true);
  assert(static_cast<bool>(high_bit >> 128) == false);

  // Contextual boolean conversions (if (a), if (!a))
  if (high_bit) {
    // Valid
  }
  else { assert(false); }

  zk::uint128_t zero_128 = 0;
  if (!zero_128) {
    // Valid
  }
  else { assert(false); }

  // 6. String edge cases
  zk::int128_t neg_val("-12345");
  assert(neg_val == -12345);
  zk::uint128_t hex_case("0xabcdef123456789ULL");
  assert(hex_case == 0xabcdef123456789ULL);
}

int main() {
  TestBasicArithmetic();
  TestComparisons();
  TestBitwise();
  TestTypePromotion();
  TestIncDec();
  TestString();
  TestEdgeCases();
  std::cout << "All C++ tests passed successfully.\n";
  return 0;
}
