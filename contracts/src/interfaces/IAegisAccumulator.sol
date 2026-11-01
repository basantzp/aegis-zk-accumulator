// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

/**
 * @title IAegisAccumulator
 * @notice Cryptographic State Accumulator Interface for Sparse Merkle Trees.
 * @author Aegis Project Contributors
 */
interface IAegisAccumulator {
    /**
     * @notice Emitted whenever a new state root is verified and committed.
     * @param previousRoot The previous state root before the update.
     * @param newRoot The new validated cryptographic state root.
     * @param batchId Monotonically increasing batch sequence number.
     * @param timestamp Block timestamp of the state transition.
     */
    event StateRootUpdated(
        bytes32 indexed previousRoot, bytes32 indexed newRoot, uint256 indexed batchId, uint256 timestamp
    );

    /**
     * @notice Verifies an inclusion or non-inclusion proof for a single leaf against a root.
     * @param root The target 32-byte Merkle root.
     * @param key The 256-bit key / state address.
     * @param value The 256-bit value committed at the key (0 for non-membership).
     * @param siblings Array of sibling hashes along the Merkle path.
     * @return isValid True if the proof cryptographically recalculates to root.
     */
    function verifyProof(bytes32 root, bytes32 key, bytes32 value, bytes32[] calldata siblings)
        external
        pure
        returns (bool isValid);

    /**
     * @notice Verifies multiple proofs in a single transaction with batched gas optimization.
     * @param root The target 32-byte Merkle root.
     * @param keys Array of 256-bit keys.
     * @param values Array of 256-bit values.
     * @param siblings Array of sibling arrays corresponding to each leaf.
     * @return allValid True if every proof in the batch is valid.
     */
    function verifyBatchProof(
        bytes32 root,
        bytes32[] calldata keys,
        bytes32[] calldata values,
        bytes32[][] calldata siblings
    ) external pure returns (bool allValid);

    /**
     * @notice Returns whether a given root is valid and part of the accumulator's history.
     * @param root The 32-byte root to query.
     */
    function isKnownRoot(bytes32 root) external view returns (bool);

    /**
     * @notice Returns the latest confirmed state root.
     */
    function currentRoot() external view returns (bytes32);

    /**
     * @notice Returns the current batch ID.
     */
    function currentBatchId() external view returns (uint256);
}
