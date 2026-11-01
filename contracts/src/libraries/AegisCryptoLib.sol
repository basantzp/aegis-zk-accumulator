// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

/**
 * @title AegisCryptoLib
 * @notice Ultra-Gas-Optimized Cryptographic Primitives and Sparse Merkle Verifier using Yul.
 * @author Aegis Project Contributors
 */
library AegisCryptoLib {
    /// @dev Emitted if sibling proof depth is invalid (0 or > 256).
    error InvalidProofDepth(uint256 depth);

    /**
     * @notice Computes parent node hash from two child hashes without memory allocations.
     * @param left Left child 32-byte hash.
     * @param right Right child 32-byte hash.
     * @return parent Resulting keccak256(left || right).
     */
    function hashNodes(bytes32 left, bytes32 right) internal pure returns (bytes32 parent) {
        assembly {
            mstore(0x00, left)
            mstore(0x20, right)
            parent := keccak256(0x00, 0x40)
        }
    }

    /**
     * @notice Computes the canonical leaf hash from key and value.
     * Non-existent/empty leaves (value == 0) evaluate to bytes32(0).
     * @param key 256-bit leaf key / address.
     * @param value 256-bit committed leaf value.
     * @return leaf Computed leaf hash.
     */
    function computeLeafHash(bytes32 key, bytes32 value) internal pure returns (bytes32 leaf) {
        if (value == bytes32(0)) {
            return bytes32(0);
        }
        assembly {
            mstore(0x00, key)
            mstore(0x20, value)
            leaf := keccak256(0x00, 0x40)
        }
    }

    /**
     * @notice Verifies a Sparse Merkle Tree inclusion or non-inclusion proof using hand-tuned Yul.
     * Operates strictly in the 64-byte EVM scratch space to guarantee zero memory expansion cost.
     *
     * @param root The target 32-byte Merkle root.
     * @param key The 256-bit key.
     * @param value The 256-bit value (0 for non-membership).
     * @param siblings Sibling hashes along the path from root down to depth.
     * @return isValid True if the recomputed root matches the provided root.
     */
    function verifyProofCalldata(bytes32 root, bytes32 key, bytes32 value, bytes32[] calldata siblings)
        internal
        pure
        returns (bool isValid)
    {
        uint256 depth = siblings.length;
        if (depth == 0 || depth > 256) {
            revert InvalidProofDepth(depth);
        }

        bytes32 current = computeLeafHash(key, value);

        assembly {
            let keyWord := key
            let siblingsOffset := siblings.offset

            // Traverse from leaf level (depth - 1) up to root (0)
            for { let i := sub(depth, 1) } 1 {} {
                // Sibling at index i in calldata
                let sibling := calldataload(add(siblingsOffset, mul(i, 0x20)))

                // Bit extraction: bit i from MSB: (key >> (255 - i)) & 1
                let shiftAmount := sub(255, i)
                let bit := and(shr(shiftAmount, keyWord), 1)

                switch bit
                case 0 {
                    // Current is left child, sibling is right child
                    mstore(0x00, current)
                    mstore(0x20, sibling)
                }
                default {
                    // Current is right child, sibling is left child
                    mstore(0x00, sibling)
                    mstore(0x20, current)
                }

                current := keccak256(0x00, 0x40)

                if iszero(i) {
                    break
                }
                i := sub(i, 1)
            }

            isValid := eq(current, root)
        }
    }

    /**
     * @notice Recomputes the Merkle root from a leaf hash and sibling path.
     * @param leafHash Precomputed leaf hash.
     * @param key Key used for direction routing.
     * @param siblings Array of sibling hashes.
     * @return computedRoot The reconstructed root.
     */
    function calculateRoot(bytes32 leafHash, bytes32 key, bytes32[] memory siblings)
        internal
        pure
        returns (bytes32 computedRoot)
    {
        uint256 depth = siblings.length;
        if (depth == 0 || depth > 256) {
            revert InvalidProofDepth(depth);
        }

        bytes32 current = leafHash;

        assembly {
            let keyWord := key
            let siblingsData := add(siblings, 0x20)

            for { let i := sub(depth, 1) } 1 {} {
                let sibling := mload(add(siblingsData, mul(i, 0x20)))
                let shiftAmount := sub(255, i)
                let bit := and(shr(shiftAmount, keyWord), 1)

                switch bit
                case 0 {
                    mstore(0x00, current)
                    mstore(0x20, sibling)
                }
                default {
                    mstore(0x00, sibling)
                    mstore(0x20, current)
                }

                current := keccak256(0x00, 0x40)

                if iszero(i) {
                    break
                }
                i := sub(i, 1)
            }

            computedRoot := current
        }
    }

    function calculateRootCalldata(bytes32 leafHash, bytes32 key, bytes32[] calldata siblings)
        internal
        pure
        returns (bytes32 computedRoot)
    {
        uint256 depth = siblings.length;
        if (depth == 0 || depth > 256) {
            revert InvalidProofDepth(depth);
        }

        bytes32 current = leafHash;

        assembly {
            let keyWord := key
            let siblingsOffset := siblings.offset

            for { let i := sub(depth, 1) } 1 {} {
                let sibling := calldataload(add(siblingsOffset, mul(i, 0x20)))
                let shiftAmount := sub(255, i)
                let bit := and(shr(shiftAmount, keyWord), 1)

                switch bit
                case 0 {
                    mstore(0x00, current)
                    mstore(0x20, sibling)
                }
                default {
                    mstore(0x00, sibling)
                    mstore(0x20, current)
                }

                current := keccak256(0x00, 0x40)

                if iszero(i) {
                    break
                }
                i := sub(i, 1)
            }

            computedRoot := current
        }
    }
}
