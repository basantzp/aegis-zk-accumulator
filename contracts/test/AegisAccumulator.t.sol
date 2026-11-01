// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import {Test, console} from "forge-std/Test.sol";
import {AegisAccumulator} from "../src/AegisAccumulator.sol";
import {AegisCryptoLib} from "../src/libraries/AegisCryptoLib.sol";

contract AegisAccumulatorTest is Test {
    AegisAccumulator public accumulator;

    address public owner = address(0x1111);
    address public sequencer = address(0x2222);
    address public attacker = address(0x9999);

    // Cross-verified test vector from core-c aegis-cli engine
    bytes32 public constant TEST_ROOT = 0xf1c8fc886a1b8973df719e3aa74a939e85449cdb6f45f2d8badd3af6ad2beaaf;
    bytes32 public constant TEST_KEY = 0x0000000000000000000000000000000000000000000000000000000000000042;
    bytes32 public constant TEST_VAL = 0x0000000000000000000000000000000000000000000000000000000000000099;

    bytes32[] internal testSiblings;

    function setUp() public {
        testSiblings = new bytes32[](8);
        testSiblings[0] = 0xffd70157e48063fc33c97a050f7f640233bf646cc98d9524c6b92bcf3ab56f83;
        testSiblings[1] = 0x887c22bd8750d34016ac3c66b5ff102dacdd73f6b014e710b51e8022af9a1968;
        testSiblings[2] = 0x0eb01ebfc9ed27500cd4dfc979272d1f0913cc9f66540d7e8005811109e1cf2d;
        testSiblings[3] = 0xe58769b32a1beaf1ea27375a44095a0d1fb664ce2dd358e7fcbfb78c26a19344;
        testSiblings[4] = 0x21ddb9a356815c3fac1026b6dec5df3124afbadb485c9ba5a3e3398a04b7ba85;
        testSiblings[5] = 0xb4c11951957c6f8f642c4af61cd6b24640fec6dc7fc607ee8206a99e92410d30;
        testSiblings[6] = 0xad3228b676f7d3cd4284a5443f17f1962b36e491b30a40b2405849e597ba5fb5;
        testSiblings[7] = 0x0000000000000000000000000000000000000000000000000000000000000000;

        vm.prank(owner);
        accumulator = new AegisAccumulator(TEST_ROOT, sequencer);
    }

    function test_InitialState() public view {
        assertEq(accumulator.currentRoot(), TEST_ROOT);
        assertEq(accumulator.sequencer(), sequencer);
        assertEq(accumulator.owner(), owner);
        assertEq(accumulator.currentBatchId(), 1);
        assertTrue(accumulator.isKnownRoot(TEST_ROOT));
    }

    function test_VerifyProof_Success() public view {
        bool valid = accumulator.verifyProof(TEST_ROOT, TEST_KEY, TEST_VAL, testSiblings);
        assertTrue(valid, "Proof from C engine should be 100% valid in Solidity Yul");
    }

    function test_VerifyProof_TamperedKey() public view {
        bytes32 tamperedKey = bytes32(uint256(TEST_KEY) ^ 0x01);
        bool valid = accumulator.verifyProof(TEST_ROOT, tamperedKey, TEST_VAL, testSiblings);
        assertFalse(valid, "Tampered key must fail verification");
    }

    function test_VerifyProof_TamperedValue() public view {
        bytes32 tamperedValue = bytes32(uint256(TEST_VAL) ^ 0x01);
        bool valid = accumulator.verifyProof(TEST_ROOT, TEST_KEY, tamperedValue, testSiblings);
        assertFalse(valid, "Tampered value must fail verification");
    }

    function test_VerifyProof_TamperedSibling() public view {
        bytes32[] memory badSiblings = testSiblings;
        badSiblings[0] = bytes32(uint256(badSiblings[0]) ^ 0xff);
        bool valid = accumulator.verifyProof(TEST_ROOT, TEST_KEY, TEST_VAL, badSiblings);
        assertFalse(valid, "Tampered sibling must fail verification");
    }

    function test_VerifyBatchProof() public view {
        bytes32[] memory keys = new bytes32[](2);
        keys[0] = TEST_KEY;
        keys[1] = TEST_KEY;

        bytes32[] memory values = new bytes32[](2);
        values[0] = TEST_VAL;
        values[1] = TEST_VAL;

        bytes32[][] memory allSiblings = new bytes32[][](2);
        allSiblings[0] = testSiblings;
        allSiblings[1] = testSiblings;

        bool batchValid = accumulator.verifyBatchProof(TEST_ROOT, keys, values, allSiblings);
        assertTrue(batchValid);
    }

    function test_UpdateRoot_Success() public {
        bytes32 newRoot = keccak256("batch_2_root");
        bytes32[] memory emptyKeys = new bytes32[](0);
        bytes32[] memory emptyValues = new bytes32[](0);
        bytes32[][] memory emptyProofs = new bytes32[][](0);

        vm.prank(sequencer);
        accumulator.updateRoot(TEST_ROOT, newRoot, emptyKeys, emptyValues, emptyProofs);

        assertEq(accumulator.currentRoot(), newRoot);
        assertEq(accumulator.currentBatchId(), 2);
        assertTrue(accumulator.isKnownRoot(TEST_ROOT));
        assertTrue(accumulator.isKnownRoot(newRoot));
    }

    function test_UpdateRoot_Unauthorized() public {
        bytes32 newRoot = keccak256("unauthorized_root");
        bytes32[] memory emptyKeys = new bytes32[](0);
        bytes32[] memory emptyValues = new bytes32[](0);
        bytes32[][] memory emptyProofs = new bytes32[][](0);

        vm.prank(attacker);
        vm.expectRevert(AegisAccumulator.Unauthorized.selector);
        accumulator.updateRoot(TEST_ROOT, newRoot, emptyKeys, emptyValues, emptyProofs);
    }

    function test_UpdateRoot_MismatchOldRoot() public {
        bytes32 wrongOldRoot = keccak256("wrong_root");
        bytes32 newRoot = keccak256("new_root");
        bytes32[] memory emptyKeys = new bytes32[](0);
        bytes32[] memory emptyValues = new bytes32[](0);
        bytes32[][] memory emptyProofs = new bytes32[][](0);

        vm.prank(sequencer);
        vm.expectRevert(abi.encodeWithSelector(AegisAccumulator.RootMismatch.selector, wrongOldRoot, TEST_ROOT));
        accumulator.updateRoot(wrongOldRoot, newRoot, emptyKeys, emptyValues, emptyProofs);
    }

    function test_TwoStepOwnershipTransfer() public {
        address newOwner = address(0x3333);

        vm.prank(owner);
        accumulator.transferOwnership(newOwner);
        assertEq(accumulator.pendingOwner(), newOwner);
        assertEq(accumulator.owner(), owner);

        // Attacker cannot accept
        vm.prank(attacker);
        vm.expectRevert(AegisAccumulator.Unauthorized.selector);
        accumulator.acceptOwnership();

        // New owner accepts
        vm.prank(newOwner);
        accumulator.acceptOwnership();
        assertEq(accumulator.owner(), newOwner);
        assertEq(accumulator.pendingOwner(), address(0));
    }

    function test_PauseEmergency() public {
        vm.prank(owner);
        accumulator.setPaused(true);
        assertTrue(accumulator.paused());

        bytes32 newRoot = keccak256("paused_root");
        bytes32[] memory emptyKeys = new bytes32[](0);
        bytes32[] memory emptyValues = new bytes32[](0);
        bytes32[][] memory emptyProofs = new bytes32[][](0);

        vm.prank(sequencer);
        vm.expectRevert(AegisAccumulator.ContractPaused.selector);
        accumulator.updateRoot(TEST_ROOT, newRoot, emptyKeys, emptyValues, emptyProofs);
    }

    function test_Constructor_ZeroRootReverts() public {
        vm.expectRevert(AegisAccumulator.InvalidRoot.selector);
        new AegisAccumulator(bytes32(0), sequencer);
    }

    function test_Constructor_ZeroSequencerReverts() public {
        vm.expectRevert(AegisAccumulator.ZeroAddress.selector);
        new AegisAccumulator(TEST_ROOT, address(0));
    }

    function test_GetHistoricalRootAt_OutOfBoundsReverts() public {
        vm.expectRevert(abi.encodeWithSelector(AegisAccumulator.IndexOutOfBounds.selector, 256, 256));
        accumulator.getHistoricalRootAt(256);
    }

    function test_UpdateRoot_DimensionMismatch_EmptyKeysNonEmptyValues() public {
        bytes32 newRoot = keccak256("new_root");
        bytes32[] memory emptyKeys = new bytes32[](0);
        bytes32[] memory nonEmptyValues = new bytes32[](1);
        nonEmptyValues[0] = bytes32(uint256(1));
        bytes32[][] memory emptyProofs = new bytes32[][](0);

        vm.prank(sequencer);
        vm.expectRevert(AegisAccumulator.InvalidBatchDimensions.selector);
        accumulator.updateRoot(TEST_ROOT, newRoot, emptyKeys, nonEmptyValues, emptyProofs);
    }

    function test_VerifyProof_Depth256_FullTree() public view {
        bytes32 key = bytes32(uint256(0x80) << 248 | 1);
        bytes32 value = bytes32(uint256(0x1234));
        bytes32[] memory siblings256 = new bytes32[](256);
        for (uint256 i = 0; i < 256; i++) {
            siblings256[i] = bytes32(i + 1);
        }

        bytes32 leafHash = AegisCryptoLib.computeLeafHash(key, value);
        bytes32 computedRoot = AegisCryptoLib.calculateRoot(leafHash, key, siblings256);

        bool valid = accumulator.verifyProof(computedRoot, key, value, siblings256);
        assertTrue(valid, "Full 256-depth tree proof must verify");
    }
}
