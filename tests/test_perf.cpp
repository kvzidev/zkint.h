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

#include <chrono>
#include <iomanip>
#include <iostream>

template<typename T>
void BenchmarkOps(const char *type_name) {
  constexpr int add_iters = 10000000;
  constexpr int mul_iters = 1000000;
  constexpr int div_iters = 100000;
  constexpr int shift_iters = 10000000;

  std::cout << "--- Benchmarking " << type_name << " ---\n";

  // 1. Addition
  {
    T num_a = 123456789;
    T num_b = 987654321;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < add_iters; ++i) { num_a += num_b; }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> n_sec = end - start;
    // Avoid optimizing away
    if (num_a == 0) { std::cout << "Check fail\n"; }
    std::cout << "  Addition:       " << std::setw(6) << std::fixed
              << std::setprecision(2) << (n_sec.count() / add_iters)
              << " ns/op\n";
  }

  // 2. Multiplication
  {
    T num_a = 12345;
    T num_b = 67890;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < mul_iters; ++i) {
      num_a *= num_b;
      // Prevent trivial constant result by adding loop index
      num_a += i;
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> n_sec = end - start;
    if (num_a == 0) { std::cout << "Check fail\n"; }
    std::cout << "  Multiplication: " << std::setw(6) << std::fixed
              << std::setprecision(2) << (n_sec.count() / mul_iters)
              << " ns/op\n";
  }

  // 3. Division
  {
    T num_a = ~T(0);
    T num_b = 123456789;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < div_iters; ++i) {
      num_a /= num_b;
      // Mutate a slightly to avoid compile optimization
      num_a += i;
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> n_sec = end - start;
    if (num_a == 0) { std::cout << "Check fail\n"; }
    std::cout << "  Division:       " << std::setw(6) << std::fixed
              << std::setprecision(2) << (n_sec.count() / div_iters)
              << " ns/op\n";
  }

  // 4. Shifts
  {
    T num_a = 1;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < shift_iters; ++i) {
      num_a <<= (i % 4);
      num_a |= 1;
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> n_sec = end - start;
    if (num_a == 0) { std::cout << "Check fail\n"; }
    std::cout << "  Shift Left:     " << std::setw(6) << std::fixed
              << std::setprecision(2) << (n_sec.count() / shift_iters)
              << " ns/op\n";
  }

  std::cout << "\n";
}

int main() {
  BenchmarkOps<zk::uint128_t>("zk::uint128_t");
  BenchmarkOps<zk::uint256_t>("zk::uint256_t");
  BenchmarkOps<zk::uint512_t>("zk::uint512_t");
  BenchmarkOps<zk::uint1024_t>("zk::uint1024_t");
  return 0;
}
