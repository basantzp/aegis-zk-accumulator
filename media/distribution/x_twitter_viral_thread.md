# 🧵 X / Twitter Engineering Thread: Aegis ZK-Accumulator

### Tweet 1 (Hook + Visual Card)
Most EVM accumulator implementations waste 45,000+ gas on recursive tree verification in pure Solidity bytecode.

We built an open-source, hybrid C11 + Solidity Sparse Merkle Tree engine with a dedicated EVM precompile shim that cuts gas by 97.5% down to 1,130 gas.

Here is the breakdown 🧵👇
[ATTACH: 03_benchmark_metrics_card.png]

### Tweet 2 (Core Architecture)
Why C11 for the core?
Stateful sparse tree operations require intensive 256-bit bitwise path traversals. In pure Solidity, storage writes and memory expansion make deep tree insertions prohibitive.

By implementing SMT-256 in C11 with O(1) zero-subtree pruning:
⚡ 0.42 μs per-leaf ingestion
⚡ 2,380,000 ops / sec on a single core
⚡ Zero dynamic leaks (verified by AddressSanitizer)

### Tweet 3 (The EVM Precompile Shim)
EVM clients (Besu, Reth, Geth) interact with native engines via precompiled contracts.
Our precompile shim at address `0x09` decodes standard 292-byte ABI calldata:
- 32-byte Root
- 32-byte Key
- 32-byte Value
- 32-byte Tree Depth word
- Array of 32-byte sibling nodes

Gas cost: flat 1,130 gas.

### Tweet 4 (The Critical Bug We Found in CI)
During precompile FFI auditing, we caught a subtle vulnerability:
Downcasting calldata[124..127] directly to a C uint32 without validating upper bytes!

An attacker sending depth = (1 << 64) + 8 would have their input truncated to 8, tricking the tree into evaluating an invalid proof as valid.

We patched it with strict 28-byte upper zero checks.
[ATTACH: 02_vulnerability_diff_vertical.png]

### Tweet 5 (The Solidity Vault & Accumulator)
On the smart contract layer:
- `AegisAccumulator.sol`: Tracks historical state in a 256-slot ring buffer with front-running resistance.
- `AegisVault.sol`: Reentrancy-guarded escrow with two-step ownership handover and pause controls.
- 100% written in modern Solidity 0.8.37.

### Tweet 6 (Testing & Invariant Rigor)
Top-tier blockchain code requires proof, not promises:
🛡️ 34/34 Foundry unit and fuzz tests pass
🛡️ 4 invariant fuzz suites (1,024 runs each, 0 failures)
🛡️ AddressSanitizer and UndefinedBehaviorSanitizer clean
🛡️ GitHub Actions CI enforces `forge fmt` and compiler matrices on every PR

### Tweet 7 (Open Source & Repo Link)
The entire project is 100% open source under the MIT license.
Language stats: 100% C and Solidity.

Check out the code, run the benchmarks, and star the repo:
🔗 https://github.com/basantzp/aegis-zk-accumulator

Built with Foundry and C11. Feedback and PRs welcome! 🚀
