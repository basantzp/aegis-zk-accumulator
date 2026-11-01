# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-11-01

### Added
- **Core C Engine (`core-c`)**:
  - Full C11 standard Keccak-256 implementation with 64-byte 2-to-1 node compression.
  - BN254 Poseidon algebraic sponge permutation (~7.2M ops/sec).
  - 256-level Sparse Merkle Tree (SMT) with Patricia-style radix zero-node pruning.
  - EVM precompile simulator matching Ethereum execution-client architecture.
  - CLI utility `aegis-cli` with `--demo`, `--export-test-vector`, and `--version`.
  - Microbenchmark suite with nanosecond-resolution timing.
  - AddressSanitizer and UndefinedBehaviorSanitizer build configurations.
- **Solidity Smart Contracts (`contracts`)**:
  - `AegisCryptoLib`: Hand-tuned Yul assembly verifier utilizing EVM scratch space (1,287 gas per proof).
  - `AegisAccumulator`: Rollup state accumulator with 256-slot ring buffer history and two-step ownership governance.
  - `AegisVault`: Layer-2 settlement vault with front-running resistance and nullifier replay protection.
  - Comprehensive Foundry test suite (24 unit and fuzz tests with 1,024 runs each).
  - Gas report profiling with detailed call telemetry.
  - Deployment script `Deploy.s.sol`.
- **Web Client & Vercel Automated Deployment (`apps/web`)**:
  - Interactive React + TypeScript + Vite dashboard with live tree visualizer.
  - Pure TypeScript Keccak-256 and SMT proof verifier.
  - ABI calldata encoder for `verifyProof(bytes32,bytes32,bytes32,bytes32[])`.
  - Side-by-side performance comparison dashboard.
  - `vercel.json` configuration for preview and production continuous deployment.
- **CI/CD & Open Source Team Essentials**:
  - Multi-compiler matrix testing in GitHub Actions (GCC & Clang).
  - Automated Vercel preview/production deployment workflow.
  - CodeQL security analysis pipeline.
  - Standard issue and PR templates, CODE_OF_CONDUCT.md, CONTRIBUTING.md, and SECURITY.md.
