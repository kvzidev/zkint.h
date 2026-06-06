# zkint

[![CI](https://github.com/kvzidev/zkint.h/actions/workflows/ci.yml/badge.svg)](https://github.com/kvzidev/zkin.ht/actions)

A high-performance, header-only multi-precision integer library for C99+ and C++11+. It implements signed and unsigned integers of sizes 128, 256, 512, and 1024 bits.

## Features

- **Header-Only**: Simple integration without pre-compilation.
- **Portability**: Compatible with GCC, Clang, MSVC, and ICC.
- **Standards Compliant**: Works with C99 and later, and C++11 and later.
- **Type Promotion**: Full C++ template-based type promotion rules matching standard behaviors.
- **Overloaded Operators**: Native arithmetic, logical, and bitwise operators in C++.
- **Generic Macros**: C11 `_Generic` macros for generic arithmetic in C.

## Installation

### CMake FetchContent

To include `zkint` in your CMake project:

```cmake
include(FetchContent)

FetchContent_Declare(
    zkint
    GIT_REPOSITORY https://github.com/kvzidev/zkint.h.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(zkint)

# Link your target
target_link_libraries(your_target PRIVATE zk::zkint)
```

## Usage

### C++ Example

```cpp
#include <zkint.h>
#include <iostream>

int main() {
    zk::uint256_t a("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
    zk::uint256_t b = 1;
    zk::uint256_t c = a + b; // Automatic promotion and overflow handling
    std::cout << c.to_string() << std::endl;
    return 0;
}
```

### C Example

```c
#include <zkint.h>
#include <stdio.h>

int main() {
    zk_uint256_t a = zk_uint256_from_string("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
    zk_uint256_t b = zk_uint256_from_uint64(1);
    zk_uint256_t c = zk_uint256_add(a, b);
    
    char buf[128];
    zk_uint256_to_string(c, buf, sizeof(buf));
    printf("%s\n", buf);
    return 0;
}
```

## Running Tests

Use the helper scripts:
- `./build.sh` to compile tests.
- `./test.sh` to execute tests.

## Compiler Compatibility

The library is verified and fully supported under the following compiler configurations:

| Compiler | Operating System | C++ Standard |
| --- | --- | --- |
| **GCC** | Linux (Ubuntu) | C++11, C++17, C++20 |
| **Clang** | Linux (Ubuntu) | C++11, C++17, C++20 |
| **Intel oneAPI (`icx`)** | Linux (Ubuntu) | C++11, C++17, C++20 |
| **MSVC (`cl`)** | Windows | C++14, C++17, C++20 |

## License

Copyright (c) 2026 Kevin Zanzi. Please see the [LICENSE](LICENSE).

