// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import {Test, console} from "forge-std/Test.sol";
import {AegisAccumulator} from "../src/AegisAccumulator.sol";
import {AegisVault} from "../src/AegisVault.sol";
import {IAegisVault} from "../src/interfaces/IAegisVault.sol";
import {AegisCryptoLib} from "../src/libraries/AegisCryptoLib.sol";

contract AegisVaultTest is Test {
    AegisAccumulator public accumulator;
    AegisVault public vault;

    address public owner = address(0x1111);
    address public sequencer = address(0x2222);
    address public alice = address(0xAAAA);
    address public bob = address(0xBBBB);
    address public mallory = address(0xCCCC);

    bytes32 public constant ACCOUNT_ID = 0x0000000000000000000000000000000000000000000000000000000000000042;
    bytes32 public constant NULLIFIER  = 0x9999000000000000000000000000000000000000000000000000000000000001;
    uint256 public constant WITHDRAW_AMOUNT = 2 ether;

    bytes32 public validRoot;
    bytes32[] internal proofSiblings;

    function setUp() public {
        vm.deal(alice, 10 ether);
        vm.deal(bob, 1 ether);
        vm.deal(mallory, 1 ether);

        // Precompute a valid 4-level Sparse Merkle path for bob's withdrawal
        proofSiblings = new bytes32[](4);
        proofSiblings[0] = 0x1111111111111111111111111111111111111111111111111111111111111111;
        proofSiblings[1] = 0x2222222222222222222222222222222222222222222222222222222222222222;
        proofSiblings[2] = 0x3333333333333333333333333333333333333333333333333333333333333333;
        proofSiblings[3] = 0x4444444444444444444444444444444444444444444444444444444444444444;

        // Leaf value binds bob, amount, and nullifier
        bytes32 leafValue = keccak256(abi.encodePacked(bob, WITHDRAW_AMOUNT, NULLIFIER));
        bytes32 leafHash = AegisCryptoLib.computeLeafHash(ACCOUNT_ID, leafValue);

        // Calculate expected root using the same Yul library
        validRoot = AegisCryptoLib.calculateRoot(leafHash, ACCOUNT_ID, proofSiblings);

        vm.startPrank(owner);
        accumulator = new AegisAccumulator(validRoot, sequencer);
        vault = new AegisVault(address(accumulator));
        vm.stopPrank();

        // Alice funds vault with 5 ETH
        vm.prank(alice);
        vault.deposit{value: 5 ether}(ACCOUNT_ID);
    }

    function test_Deposit_Success() public {
        assertEq(address(vault).balance, 5 ether);

        vm.prank(alice);
        vm.expectEmit(true, true, false, true);
        emit IAegisVault.Deposited(alice, ACCOUNT_ID, 1 ether);
        vault.deposit{value: 1 ether}(ACCOUNT_ID);

        assertEq(address(vault).balance, 6 ether);
    }

    function test_Deposit_ZeroAmountReverts() public {
        vm.prank(alice);
        vm.expectRevert(AegisVault.ZeroAmount.selector);
        vault.deposit{value: 0}(ACCOUNT_ID);
    }

    function test_Withdraw_Success() public {
        uint256 bobInitialBalance = bob.balance;

        vm.prank(bob);
        vm.expectEmit(true, true, false, true);
        emit IAegisVault.Withdrawn(bob, ACCOUNT_ID, WITHDRAW_AMOUNT, NULLIFIER);

        vault.withdraw(validRoot, ACCOUNT_ID, WITHDRAW_AMOUNT, NULLIFIER, proofSiblings);

        assertEq(bob.balance, bobInitialBalance + WITHDRAW_AMOUNT);
        assertEq(address(vault).balance, 3 ether);
        assertTrue(vault.isNullifierUsed(NULLIFIER));
    }

    function test_Withdraw_ReplayReverts() public {
        vm.prank(bob);
        vault.withdraw(validRoot, ACCOUNT_ID, WITHDRAW_AMOUNT, NULLIFIER, proofSiblings);

        // Replay attempt
        vm.prank(bob);
        vm.expectRevert(abi.encodeWithSelector(AegisVault.NullifierAlreadySpent.selector, NULLIFIER));
        vault.withdraw(validRoot, ACCOUNT_ID, WITHDRAW_AMOUNT, NULLIFIER, proofSiblings);
    }

    function test_Withdraw_TamperedAmountReverts() public {
        uint256 badAmount = WITHDRAW_AMOUNT + 1 ether;

        vm.prank(bob);
        vm.expectRevert(AegisVault.InvalidWithdrawalProof.selector);
        vault.withdraw(validRoot, ACCOUNT_ID, badAmount, NULLIFIER, proofSiblings);
    }

    function test_Withdraw_FrontRunProtection() public {
        // Mallory attempts to submit Bob's proof to steal Bob's withdrawal
        vm.prank(mallory);
        vm.expectRevert(AegisVault.InvalidWithdrawalProof.selector);
        vault.withdraw(validRoot, ACCOUNT_ID, WITHDRAW_AMOUNT, NULLIFIER, proofSiblings);
    }

    function test_Withdraw_InvalidRootReverts() public {
        bytes32 uncommittedRoot = keccak256("unknown_root");

        vm.prank(bob);
        vm.expectRevert(abi.encodeWithSelector(AegisVault.InvalidStateRoot.selector, uncommittedRoot));
        vault.withdraw(uncommittedRoot, ACCOUNT_ID, WITHDRAW_AMOUNT, NULLIFIER, proofSiblings);
    }

    function test_Withdraw_InsufficientLiquidityReverts() public {
        // Drain vault to 1 ether
        vm.prank(owner);
        vault.emergencyDrain(owner, 4 ether);

        // Bob requests 2 ether but vault only has 1 ether
        vm.prank(bob);
        vm.expectRevert(abi.encodeWithSelector(AegisVault.InsufficientLiquidity.selector, WITHDRAW_AMOUNT, 1 ether));
        vault.withdraw(validRoot, ACCOUNT_ID, WITHDRAW_AMOUNT, NULLIFIER, proofSiblings);
    }

    function test_EmergencyPause_BlocksWithdrawals() public {
        vm.prank(owner);
        vault.setPaused(true);

        vm.prank(bob);
        vm.expectRevert(AegisVault.VaultPaused.selector);
        vault.withdraw(validRoot, ACCOUNT_ID, WITHDRAW_AMOUNT, NULLIFIER, proofSiblings);
    }
}
