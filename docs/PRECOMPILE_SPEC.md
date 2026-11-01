# EIP-Style Specification: Aegis SMT Verification Precompile

```
Precompile Address: 0x20 (or assigned testnet precompile)
Gas Cost: 450 + 85 * depth
Input Size: 128 + 32 * depth bytes
Output Size: 32 bytes
```

## 1. Input Layout

The precompile input expects big-endian ABI calldata:

| Offset | Length | Description | Type |
| :--- | :--- | :--- | :--- |
| `0x00` | 32 | Target State Root | `bytes32` |
| `0x20` | 32 | Leaf Key | `bytes32` |
| `0x40` | 32 | Committed Leaf Value | `bytes32` |
| `0x60` | 32 | Proof Depth $N$ | `uint256` |
| `0x80` | $32 \times N$ | Sibling Hashes | `bytes32[N]` |

## 2. Output Layout

The precompile returns exactly 32 bytes:
- `0x0000000000000000000000000000000000000000000000000000000000000001` if the proof is cryptographically sound and recalculates to the target state root.
- `0x0000000000000000000000000000000000000000000000000000000000000000` if the proof fails or input data is malformed.

## 3. Gas Schedule

$$\text{Gas} = \text{BaseGas} + N \times \text{PerLevelGas}$$
$$\text{Gas} = 450 + 85 \times N$$

For a standard 32-level rollup tree:
$$\text{Gas} = 450 + 85 \times 32 = 3,170 \text{ gas}$$

This provides a ~70% discount compared to interpreted EVM execution, making large-scale state transitions and stateless verifications economically viable.
