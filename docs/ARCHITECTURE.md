# Aegis Architecture & Systems Design

The Aegis ZK Accumulator is engineered as a hybrid off-chain / on-chain cryptographic infrastructure suite designed for high-throughput Layer 2 rollups, cross-chain bridges, and stateless EVM validation.

```
+-------------------------------------------------------------------------------+
|                             AEGIS ZK ACCUMULATOR                              |
+-------------------------------------------------------------------------------+
|                                                                               |
|   +--------------------------+                 +--------------------------+   |
|   |       CORE C ENGINE      |                 |    SOLIDITY CONTRACTS    |   |
|   |  - C11 Zero-Copy Sponge  |                 |  - Yul Scratch Verifier  |   |
|   |  - 395K Keccak ops/s     |  Export Proofs  |  - 1,287 gas / proof     |   |
|   |  - 7.2M Poseidon ops/s   | --------------> |  - Ring Buffer Roots     |   |
|   |  - Radix Pruned SMT      |   & Calldata    |  - MEV-Proof Settlement  |   |
|   |  - Precompile ABI Engine |                 |  - IAegisVault Layer     |   |
|   +--------------------------+                 +--------------------------+   |
|                 |                                            |                |
|                 +--------------------+-----------------------+                |
|                                      |                                        |
|                                      v                                        |
|                       +-----------------------------+                         |
|                       |    VERCEL WEB EXPLORER      |                         |
|                       |  - Interactive SMT Explorer |                         |
|                       |  - In-Browser TS Keccak-256 |                         |
|                       |  - 1-Click Calldata & Cast  |                         |
|                       |  - Automated CI/CD Deploy   |                         |
|                       +-----------------------------+                         |
|                                                                               |
+-------------------------------------------------------------------------------+
```

## 1. Core C Engine Model

The C engine provides maximum throughput for sequencer nodes, state provers, and node precompiles:
- **Zero Heap Allocations**: The 2-to-1 node compression function `aegis_hash_nodes` operates entirely within CPU registers and L1 cache lines (64-byte blocks), avoiding `malloc` on the critical path.
- **Patricia Radix Pruning**: Empty subtrees are not materialized in memory. Instead, a static array of precomputed zero hashes is referenced, reducing memory complexity from $O(2^D)$ to $O(N)$ where $N$ is the count of inserted keys.
- **Constant-Time Verification**: Verification of a proof traverses exactly $D$ steps with deterministic branchless bit extraction.

## 2. On-Chain Solidity Verification

- **Scratch Space Utilization**: Standard EVM contracts write to memory using `abi.encode`, triggering memory expansion costs ($O(N^2)$ gas beyond 724 bytes). `AegisCryptoLib` executes strictly within the EVM scratch space (`0x00..0x40`), eliminating memory expansion costs.
- **Circular Ring Buffer**: `AegisAccumulator` tracks the latest 256 state roots in a constant-sized ring buffer. This provides a temporal window for users to submit withdrawal proofs without timing out or failing due to race conditions with sequencer updates.

## 3. Front-Running Resistant Vault

Rollup settlement contracts frequently suffer from front-running attacks on public mempools, where MEV bots observe an honest user's withdrawal proof and re-submit it with their own address.
`AegisVault` prevents this by binding `msg.sender` directly into the leaf preimage:
$$\text{LeafValue} = \text{keccak256}(\text{msg.sender} \parallel \text{amount} \parallel \text{nullifier})$$
Any attempt by a third party to submit the transaction will fail cryptographic verification because `msg.sender` diverges from the leaf commitment.
