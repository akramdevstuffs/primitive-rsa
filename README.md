# primitive-rsa

A from-scratch implementation of the RSA cryptosystem in C, inspired by the original research paper, [*A Method for Obtaining Digital Signatures and Public-Key Cryptosystems*](https://people.csail.mit.edu/rivest/Rsapaper.pdf), by Ron Rivest, Adi Shamir, and Leonard Adleman.

The goal of **primitive-rsa** is to explore the fundamentals of RSA and develop an efficient implementation in C without relying on existing cryptographic libraries for its core operations.

## Motivation

RSA is one of the foundational public-key cryptosystems, built on the mathematical properties of prime numbers and modular arithmetic.

This project aims to understand and implement the algorithm from the ground up, focusing on its underlying mathematics, low-level implementation details, and computational efficiency.

## Project Status

**primitive-rsa** is in its early stages of development. Its implementation, features, and performance are subject to change.

## Security Disclaimer

**This project is not intended for production use.**

- It does not claim compliance with cryptographic standards such as PKCS #1 (RFC 8017).
- It provides no security guarantees.
- It may lack essential security mechanisms, including secure padding, cryptographically secure random number generation, and protections against side-channel attacks.
- It has not necessarily undergone independent security audits.

The goal is to explore the RSA algorithm and its implementation, not to provide a secure cryptographic solution. Do not use this project to protect sensitive data or secure real-world communications.

For production cryptography, use a well-established cryptographic library with appropriate standards support and security review.

## Reference

- Rivest, R. L., Shamir, A., and Adleman, L. (1978). [A Method for Obtaining Digital Signatures and Public-Key Cryptosystems](https://people.csail.mit.edu/rivest/Rsapaper.pdf).

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
