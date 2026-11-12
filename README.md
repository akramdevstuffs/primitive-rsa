# primitive-rsa

[![C17](https://img.shields.io/badge/standard-C17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build & Tests](https://img.shields.io/badge/tests-passing-brightgreen.svg)]()

A from-scratch implementation of the RSA cryptosystem in standard C (C17), inspired by the seminal 1978 research paper [*A Method for Obtaining Digital Signatures and Public-Key Cryptosystems*](https://people.csail.mit.edu/rivest/Rsapaper.pdf) by Ron Rivest, Adi Shamir, and Leonard Adleman.

The goal of **primitive-rsa** is to explore RSA and arbitrary-precision modular arithmetic from the ground up—implementing big-integer arithmetic, primality testing, and key derivation without external cryptographic libraries (such as OpenSSL or GMP).

---

## Features

- **Custom BigInt Engine**:
  - Dynamically sized 32-bit limb representation.
  - Fast multiplication using Fast Fourier Transform (FFT / NTT) alongside schoolbook multiplication.
  - Arbitrary-precision division and modulo using Knuth's Algorithm D.
  - Modular exponentiation optimized via Montgomery reduction.
  - Byte array (`uint8_t*`) and decimal string serialization/deserialization.
- **Number Theory & Key Generation**:
  - Miller-Rabin probabilistic primality testing.
  - Random prime generation with arbitrary bit lengths.
  - Extended Euclidean Algorithm for Greatest Common Divisor ($\gcd$) and modular inverse computation ($e \cdot d \equiv 1 \pmod{\phi(n)}$).
- **RSA Operations**:
  - Key pair generation (`rsa_keygen`).
  - Raw / textbook RSA encryption and decryption (`crt_encrypt`, `crt_decrypt`).
  - Original paper-faithful dual sign-and-encrypt protocol (`rsa_encrypt`, `rsa_decrypt`) providing simultaneous confidentiality and authentication.
- **Zero External Dependencies**: Pure C17 standard library and standard math library.

---

## Project Structure

```
primitive-rsa-c/
├── include/
│   ├── bigint/          # BigInt data structure and arithmetic headers
│   └── rsa/             # RSA, key generation, algorithms, and debug headers
├── src/
│   ├── bigint/          # BigInt implementation (FFT, Knuth div, Montgomery)
│   ├── algorithms.c     # Miller-Rabin, Extended Euclidean GCD, prime generator
│   ├── key_generator.c  # RSA p, q, n, e, d generation
│   ├── crypt.c          # Core modular exponentiation encryption/decryption
│   ├── rsa.c            # High-level RSA keygen, sign & encrypt workflows
│   └── main.c           # Interactive demonstration program
├── tests/
│   ├── bigint/          # Unit tests for BigInt operations
│   └── rsa/             # Tests for key generation, algorithms, and RSA flows
├── docs/
│   └── ARCHITECTURE.md  # Detailed technical design of each layer
└── CMakeLists.txt       # Build system configuration
```

---

## Getting Started

### Prerequisites

- A C17-compliant C compiler (`gcc` >= 9 or `clang` >= 10)
- `cmake` >= 3.20
- `ninja` or `make`

### Building

```bash
# Configure the build directory
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Compile the project and tests
cmake --build build
```

### Running the Demo

Execute the demo binary to see key generation, standard encryption, and paper-faithful sign-and-encrypt in action:

```bash
./build/rsa
```

### Running Tests

The test suite validates BigInt arithmetic (addition, subtraction, FFT multiplication, Knuth division, Montgomery modpow) and RSA routines:

```bash
ctest --test-dir build --output-on-failure
```

---

## Quick API Example

```c
#include "rsa/rsa.h"
#include "rsa/crypt.h"
#include "bigint/bigint.h"

// 1. Generate 256-bit key pair
rsa_private_key_t *prk = NULL;
rsa_public_key_t  *pbk = NULL;
rsa_keygen(&prk, &pbk, 256);

// 2. Convert plaintext to BigInt (aligned to 4-byte limb boundary)
const char text[] = "Hello, RSA!"; // 12 bytes
bigint_t *msg = bigint_from_bytes((const uint8_t *)text, sizeof(text));

// 3. Encrypt message: c = m^e mod n
bigint_t *cipher = crt_encrypt(pbk->e, pbk->n, msg);

// 4. Decrypt message: m = c^d mod n
bigint_t *decrypted = crt_decrypt(prk->d, prk->n, cipher);

// 5. Restore plaintext bytes
char output[32] = {0};
bigint_to_bytes(decrypted, (uint8_t *)output, sizeof(output));

// 6. Free resources
bigint_free(decrypted);
bigint_free(cipher);
bigint_free(msg);
rsa_free_pbk(pbk);
rsa_free_prk(prk);
```

For more architectural details, see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

---

## Security Disclaimer

> [!WARNING]
> **This project is strictly for educational and research purposes.**
>
> - It is **not** intended for production use.
> - It does **not** implement PKCS #1 v2.2 (OAEP/PSS) padding schemes and uses unpadded ("textbook") RSA arithmetic.
> - It does not provide side-channel (timing/cache) attack mitigations or a cryptographically secure entropy source (`CSPRNG`).
> - For production security requirements, always use reviewed libraries such as OpenSSL, BoringSSL, or libsodium.

---

## Reference

- Rivest, R. L., Shamir, A., and Adleman, L. (1978). [A Method for Obtaining Digital Signatures and Public-Key Cryptosystems](https://people.csail.mit.edu/rivest/Rsapaper.pdf). *Communications of the ACM*, 21(2), 120–126.

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
