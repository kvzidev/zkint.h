# Project Architecture: zkint

`zkint` is a header-only multi-precision integer library supporting C99+ and C++11+. It provides signed and unsigned 128, 256, 512, and 1024-bit integers.

## Binary Representation

Each big integer is represented by an array of 64-bit limbs.
- `zk_uint128_t` / `zk_int128_t`: `uint64_t limbs[2]`
- `zk_uint256_t` / `zk_int256_t`: `uint64_t limbs[4]`
- `zk_uint512_t` / `zk_int512_t`: `uint64_t limbs[8]`
- `zk_uint1024_t` / `zk_int1024_t`: `uint64_t limbs[16]`

Memory is ordered in little-endian order (least significant limb at index 0).

## C interface (`zkint_c.h`)

All C types are defined as structures containing a raw array of limbs.
Functions use prefixes to prevent namespace collisions. For example, `zk_uint256_add(zk_uint256_t a, zk_uint256_t b)` performs addition on 256-bit unsigned integers.
For C11 and later, `zk_add`, `zk_sub`, `zk_mul`, `zk_div`, etc., are provided via `_Generic` macros mapping to correct typed functions based on arguments.

## C++ wrapper (`zkint_cpp.h`)

Under C++, the types `zk::uint128_t`, `zk::int128_t`, etc., are defined using a generic template class `zk::bigint<size_t Bits, bool Signed>`.
This template class wraps the equivalent C struct (e.g., `zk_uint128_t`) as its underlying data layout, ensuring perfect binary compatibility.
C++ operators are overloaded for all common arithmetic, comparison, and bitwise operations.

### Type Promotion Rules
For binary operations between two types `T1` and `T2`:
- If both are `zk::bigint` types:
  - Bit-width of result is `max(Bits1, Bits2)`.
  - Signedness is:
    - If `Bits1 != Bits2`: signedness of the larger type.
    - If `Bits1 == Bits2`: signed if both are signed, unsigned if either is unsigned.
- If one is a standard C++ integer and the other is a `zk::bigint`:
  - The standard integer is implicitly promoted to a `zk::bigint` of standard size (e.g., 64-bit) and signedness, then the rules above apply.

## Algorithmic Implementation

### Addition and Subtraction
Implemented using standard limb-by-limb ripple-carry addition and subtraction with borrow. Compiler carry/borrow intrinsics (like `_addcarry_u64` and `_subborrow_u64` on MSVC) are used if available, otherwise a portable C implementation is used.

### Multiplication
Schoolbook multiplication $O(N^2)$ is used. Since the limb count is small (maximum 16 limbs), this is highly efficient. Multiplications of limbs use standard `unsigned __int128` on platforms where it is available, otherwise splitting limb values into 32-bit halves.

### Division and Modulo
Bit-by-bit division (restoring division) is used for multi-limb divisors. For single-limb divisors, a fast-path is implemented using native 128-bit/64-bit division instructions or compiler intrinsics (like `_udiv128` or native `unsigned __int128` division). A carry-zero bypass is employed to optimize execution on sparse arrays.
