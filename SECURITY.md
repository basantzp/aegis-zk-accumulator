# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |
| < 1.0.0 | :x:                |

---

## Threat Model & Invariants

Aegis ZK Accumulator is engineered for high-security rollup state commitments and settlement vaults. The system maintains the following formal invariants:

1. **Cryptographic Soundness**: A Sparse Merkle Tree inclusion proof cannot be forged without computing a second preimage for Keccak-256 or Poseidon.
2. **Front-Running Resistance**: On-chain withdrawals in `AegisVault` bind the recipient (`msg.sender`), amount, and random nullifier directly into the leaf value. Front-running MEV bots cannot divert funds by intercepting mempool proofs.
3. **Double-Spend Prevention**: Every withdrawal permanently commits a 32-byte nullifier to storage. Replaying the same proof reverts immediately.
4. **Historical Root Validity**: Only roots committed to the 256-slot circular ring buffer by an authorized sequencer are recognized as valid state anchors.
5. **Memory Safety**: The C11 engine guarantees zero buffer overflows, memory leaks, and out-of-bounds reads, verified continuously via AddressSanitizer and UndefinedBehaviorSanitizer.

---

## Reporting a Vulnerability

If you discover a security vulnerability, please do NOT file a public issue.

Send reports to: **security@aegis-crypto.org** or use the GitHub Security Advisory feature.

Include:
- Summary of the vulnerability and attack vector
- Proof of Concept (PoC) script or failing Foundry test
- Assessment of potential impact

We will respond within 48 hours to acknowledge receipt and coordinate a disclosure timeline.
