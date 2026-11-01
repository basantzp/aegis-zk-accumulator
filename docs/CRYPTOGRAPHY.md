# Cryptographic Specification

## 1. Keccak-256 Sponge Permutation

Aegis implements standard Ethereum Keccak-256 conforming to FIPS 202:
- State size: $b = 1600$ bits (25 64-bit words).
- Rate: $r = 1088$ bits ($136$ bytes).
- Capacity: $c = 512$ bits ($64$ bytes).
- Padding: Delimiter `0x01` followed by zeroes and terminating in `0x80`.

For 2-to-1 Merkle tree node hashing:
$$\text{Parent} = \text{keccak256}(\text{Left} \parallel \text{Right})$$
Input size is exactly 64 bytes, which fits comfortably inside a single 136-byte rate block, requiring exactly one call to `keccak_f1600`.

## 2. Poseidon BN254 Permutation

For ZK-SNARK circuit compatibility, Aegis incorporates an algebraic Poseidon permutation over the BN254 scalar field:
$$p = 21888242871839275222246405745257275088548364400416034343698204186575808495617$$
- State width $t = 4$.
- S-Box: $S(x) = x^5 \pmod p$.
- Full rounds $R_F = 8$, partial rounds $R_P = 56$.

## 3. Sparse Merkle Tree (SMT) Path Traversal

Each key $K \in \{0, 1\}^{256}$ defines a path from the root down to the leaf:
- Let $K_i$ be bit $i$ of key $K$ (where $i = 0$ is the MSB).
- If $K_i = 0$, the path traverses to the left child, and the sibling is on the right.
- If $K_i = 1$, the path traverses to the right child, and the sibling is on the left.

### Precomputed Zero Hashes
To support large depths (e.g. 256) efficiently:
$$Z_0 = 0x0000000000000000000000000000000000000000000000000000000000000000$$
$$Z_d = \text{hashNodes}(Z_{d-1}, Z_{d-1}) \quad \forall d \in [1, 256]$$

This mathematical formulation guarantees that non-membership proofs (verifying that a key has value 0) are indistinguishable in structure from membership proofs.
