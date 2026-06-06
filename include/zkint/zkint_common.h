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

#ifndef ZKINT_COMMON_H
#define ZKINT_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_MSC_VER)
#define ZKINT_INLINE static __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define ZKINT_INLINE static inline __attribute__((always_inline))
#else
#define ZKINT_INLINE static inline
#endif

#if defined(__SIZEOF_INT128__)
#define ZKINT_HAS_INT128 1
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_addcarry_u64)
#if defined(_M_X64) || defined(_M_ARM64)
#define ZKINT_MSVC_64BIT 1
#endif
#endif

#endif /* ZKINT_COMMON_H */
