// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import {Test, console} from "forge-std/Test.sol";
import {AegisCryptoLib} from "../src/libraries/AegisCryptoLib.sol";
import {AegisAccumulator} from "../src/AegisAccumulator.sol";

contract AegisFuzzTest is Test {
    AegisAccumulator public accumulator;

    function setUp() public {
        accumulator = new AegisAccumulator(bytes32(uint256(1)), address(0x2222));
    }

    /// @notice Property: Distinct non-zero keys with same value must produce distinct leaf hashes
    function testFuzz_DifferentKeysProduceDifferentLeaves(bytes32 k1, bytes32 k2, bytes32 val) public pure {
        vm.assume(k1 != k2);
        vm.assume(val != bytes32(0));

        bytes32 leaf1 = AegisCryptoLib.computeLeafHash(k1, val);
        bytes32 leaf2 = AegisCryptoLib.computeLeafHash(k2, val);

        assertTrue(leaf1 != leaf2, "Collision in leaf hashing");
    }

    /// @notice Property: Empty leaf (value == 0) always maps to bytes32(0) regardless of key
    function testFuzz_EmptyLeafIsAlwaysZero(bytes32 key) public pure {
        bytes32 leaf = AegisCryptoLib.computeLeafHash(key, bytes32(0));
        assertEq(leaf, bytes32(0), "Empty leaf must be 0x0");
    }

    /// @notice Property: Reconstructed root from a proof and valid leaf is uniquely deterministic
    function testFuzz_RootCalculationDeterministic(bytes32 leaf, bytes32 key, bytes32 sib1, bytes32 sib2) public pure {
        bytes32[] memory siblings = new bytes32[](2);
        siblings[0] = sib1;
        siblings[1] = sib2;

        bytes32 root1 = AegisCryptoLib.calculateRoot(leaf, key, siblings);
        bytes32 root2 = AegisCryptoLib.calculateRoot(leaf, key, siblings);

        assertEq(root1, root2, "Root calculation must be strictly deterministic");
    }

    /// @notice Property: Flipping any bit in a sibling hash guarantees verification failure
    function testFuzz_BitFlipBreaksVerification(bytes32 key, bytes32 val, bytes32 sib0, bytes32 sib1, uint8 bitToFlip)
        public
        view
    {
        vm.assume(val != bytes32(0));
        bitToFlip = uint8(bound(bitToFlip, 0, 255));

        bytes32[] memory siblings = new bytes32[](2);
        siblings[0] = sib0;
        siblings[1] = sib1;

        bytes32 leaf = AegisCryptoLib.computeLeafHash(key, val);
        bytes32 validRoot = AegisCryptoLib.calculateRoot(leaf, key, siblings);

        // Precondition: the unmodified proof verifies
        assertTrue(accumulator.verifyProof(validRoot, key, val, siblings));

        // Corrupt sibling 0 by flipping exactly one bit
        siblings[0] = bytes32(uint256(siblings[0]) ^ (1 << bitToFlip));

        // Invariant: corrupted proof MUST fail
        assertFalse(
            accumulator.verifyProof(validRoot, key, val, siblings), "Corrupted proof unexpectedly passed verification"
        );
    }
}
