# Contributing to Aegis ZK Accumulator

Thank you for your interest in contributing to Aegis! This project merges low-level systems programming in C11 with gas-optimized Solidity smart contracts and a modern Vercel-deployed web explorer.

---

## Architecture Overview

1. **`core-c/`**: High-performance cryptographic primitives (Keccak-256, Poseidon sponge permutation), Patricia-style Sparse Merkle Tree (SMT), EVM precompile simulator, and CLI tool.
2. **`contracts/`**: Foundry-based smart contracts (`AegisAccumulator`, `AegisVault`, `AegisCryptoLib`). Yul assembly for scratch-space verification.
3. **`apps/web/`**: Interactive Vite + React + TypeScript explorer configured for zero-config automated Vercel deployment.
4. **`docs/`**: Cryptographic specifications, gas analysis, and formal proofs.

---

## Development Setup

### Prerequisites
- GCC (>= 11) or Clang (>= 14)
- Make or CMake (>= 3.16)
- [Foundry](https://getfoundry.sh/) (`forge`, `cast`)
- Node.js (>= 20) & npm

### Build All Components
```bash
# Clone the repository
git clone https://github.com/basantzp/aegis-zk-accumulator.git
cd aegis-zk-accumulator

# Build everything
make all
```

### Running Tests
```bash
# Run all tests (C + Solidity)
make test

# Run C under AddressSanitizer and UndefinedBehaviorSanitizer
make asan

# Run Foundry tests with detailed gas reports
cd contracts && forge test --gas-report
```

---

## Code Style & Standards

### C Code
- Standard: **C11** (`-std=c11 -Wall -Wextra -Werror -pedantic`).
- Formatting: Follow `.clang-format` (LLVM style).
- Memory Safety: Zero heap allocations on the hot verification path. Zero memory leaks (`make asan`).

### Solidity Code
- Solidity: **^0.8.20** (tested on 0.8.28).
- Formatting: Follow `forge fmt`.
- Assembly: Document memory layout in Yul blocks; ensure scratch-space safety (`0x00..0x40`).
- NatSpec: Document `@notice`, `@param`, `@return` on all public interfaces.

### Git Conventions
We adhere to [Conventional Commits](https://www.conventionalcommits.org/):
- `feat(core-c): add SIMD acceleration for keccak sponge`
- `perf(contracts): optimize scratch space bit shifts in Yul`
- `fix(web): resolve edge case in sibling path rendering`
- `docs(spec): update EVM precompile gas scheduling table`

---

## Pull Request Process

1. Fork the repo and create a topic branch (`git checkout -b feat/my-feature`).
2. Implement your changes following coding standards.
3. Verify that all tests pass (`make test && make asan`).
4. Ensure PR template checklist is completed.
5. Submit PR targeting `main`.
