# Architecture & Design

`primitive-rsa` is structured in three modular layers, designed from scratch without external cryptographic or arbitrary-precision libraries.

```
+-----------------------------------------------------------+
|                        RSA Layer                          |
|   (rsa_keygen, rsa_encrypt, rsa_decrypt, crt_*)           |
+-----------------------------------------------------------+
                             |
                             v
+-----------------------------------------------------------+
|                    Algorithms Layer                       |
|   (Miller-Rabin, Extended Euclidean GCD, Prime Search)     |
+-----------------------------------------------------------+
                             |
                             v
+-----------------------------------------------------------+
|                      BigInt Engine                        |
|   (32-bit limbs, FFT / NTT multiplication, Knuth Div D,   |
|               Montgomery Modpow, Shifts)                  |
+-----------------------------------------------------------+
```

---

## 1. BigInt Engine (`include/bigint/bigint.h`, `src/bigint/`)

Represents arbitrary-precision unsigned and signed integers using dynamically allocated 32-bit limbs.

- **Storage**: `bigint_t` tracks a `uint32_t *limbs` buffer, active `size`, allocated `capacity`, and sign bit `negative`.
- **Multiplication**:
  - Schoolbook multiplication $O(N^2)$ for small inputs.
  - Fast Fourier Transform (FFT / NTT) multiplication $O(N \log N)$ for large integers.
- **Division**:
  - Single-limb fast division for 32-bit and 64-bit divisors.
  - Knuth's Algorithm D for arbitrary multi-limb division and modulo.
- **Modular Exponentiation**:
  - Implements Montgomery reduction for efficient repeated modular multiplication during primality tests and RSA encryption/decryption.

---

## 2. Algorithms & Number Theory (`include/rsa/algorithms.h`, `src/algorithms.c`)

Provides the mathematical machinery needed for RSA key generation:

- **Miller-Rabin Primality Test**: Probabilistic primality test to verify generated candidate primes.
- **Prime Generation**: Searches for prime numbers of target bit lengths within random bounds $[lb, ub]$.
- **Extended Euclidean Algorithm**: Computes $\gcd(a, b)$ and Bézout coefficients $x, y$ such that $a \cdot x + b \cdot y = \gcd(a, b)$, used to compute modular multiplicative inverses ($e \cdot d \equiv 1 \pmod{\phi(N)}$).

---

## 3. RSA Cryptosystem (`include/rsa/`, `src/rsa.c`, `src/crypt.c`, `src/key_generator.c`)

Implements the RSA protocol based on Rivest, Shamir, and Adleman (1978):

- **Key Generation (`rsa_keygen`)**:
  1. Generates two distinct large primes $p$ and $q$.
  2. Calculates modulus $n = p \times q$.
  3. Calculates Euler's totient $\phi(n) = (p - 1)(q - 1)$.
  4. Selects private exponent $d$ and derives public exponent $e = d^{-1} \pmod{\phi(n)}$ (or vice-versa).
- **Core Operations (`crt_encrypt`, `crt_decrypt`)**:
  - Encryption: $c = m^e \pmod{n}$
  - Decryption: $m = c^d \pmod{n}$
- **Dual Sign-and-Encrypt (`rsa_encrypt`, `rsa_decrypt`)**:
  - Faithful to Section VII of the 1978 paper: sender signs with private key, then encrypts with recipient's public key for simultaneous privacy and authentication.

---

## Memory Management

- Functions that return newly allocated `bigint_t*` transfer ownership to the caller. Call `bigint_free()` to release them.
- RSA key pairs are allocated via `rsa_keygen()` and released using `rsa_free_prk()` and `rsa_free_pbk()`.
