// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import {IAegisVault} from "./interfaces/IAegisVault.sol";
import {IAegisAccumulator} from "./interfaces/IAegisAccumulator.sol";

/**
 * @title AegisVault
 * @notice Trustless Layer-2 Settlement Vault backed by Aegis Cryptographic Accumulator.
 * @dev Front-running resistant: leaf commitment binds msg.sender, amount, and nullifier.
 * @author Aegis Project Contributors
 */
contract AegisVault is IAegisVault {
    /// @notice Cryptographic Accumulator tracking verified rollup state roots.
    IAegisAccumulator public immutable accumulator;

    /// @notice Vault administrator.
    address public owner;

    /// @notice Pending owner for safe two-step ownership transfers.
    address public pendingOwner;

    /// @notice Emergency pause flag.
    bool public paused;

    /// @notice Nullifier registry preventing replay attacks and double withdrawals.
    mapping(bytes32 => bool) public override isNullifierUsed;

    /// @dev Events
    event OwnershipTransferStarted(address indexed previousOwner, address indexed newOwner);
    event OwnershipTransferred(address indexed previousOwner, address indexed newOwner);
    event EmergencyDrained(address indexed recipient, uint256 amount);
    event PausedStateChanged(bool isPaused);

    /// @dev Reentrancy lock status.
    uint256 private constant _NOT_ENTERED = 1;
    uint256 private constant _ENTERED = 2;
    uint256 private _status;

    /// @dev Custom errors for gas minimization.
    error ReentrancyGuard();
    error VaultPaused();
    error ZeroAmount();
    error NullifierAlreadySpent(bytes32 nullifier);
    error InvalidStateRoot(bytes32 root);
    error InsufficientLiquidity(uint256 requested, uint256 available);
    error InvalidWithdrawalProof();
    error NativeTransferFailed();
    error Unauthorized();
    error ZeroAddress();

    modifier nonReentrant() {
        if (_status == _ENTERED) revert ReentrancyGuard();
        _status = _ENTERED;
        _;
        _status = _NOT_ENTERED;
    }

    modifier onlyOwner() {
        if (msg.sender != owner) revert Unauthorized();
        _;
    }

    modifier whenNotPaused() {
        if (paused) revert VaultPaused();
        _;
    }

    /**
     * @notice Deploy the AegisVault bound to an AegisAccumulator contract.
     * @param _accumulator Address of the deployed IAegisAccumulator instance.
     */
    constructor(address _accumulator) {
        if (_accumulator == address(0)) revert ZeroAddress();
        accumulator = IAegisAccumulator(_accumulator);
        owner = msg.sender;
        _status = _NOT_ENTERED;
    }

    /**
     * @inheritdoc IAegisVault
     */
    function deposit(bytes32 accountId) external payable override whenNotPaused {
        if (msg.value == 0) revert ZeroAmount();
        emit Deposited(msg.sender, accountId, msg.value);
    }

    /**
     * @inheritdoc IAegisVault
     */
    function withdraw(bytes32 root, bytes32 accountId, uint256 amount, bytes32 nullifier, bytes32[] calldata proof)
        external
        override
        nonReentrant
        whenNotPaused
    {
        if (amount == 0) revert ZeroAmount();
        if (isNullifierUsed[nullifier]) revert NullifierAlreadySpent(nullifier);
        if (!accumulator.isKnownRoot(root)) revert InvalidStateRoot(root);
        if (address(this).balance < amount) revert InsufficientLiquidity(amount, address(this).balance);

        // Bind recipient msg.sender into the cryptographic leaf value to prevent MEV front-running
        bytes32 leafValue = keccak256(abi.encodePacked(msg.sender, amount, nullifier));

        // Verify cryptographic Merkle proof against historical accumulator root
        if (!accumulator.verifyProof(root, accountId, leafValue, proof)) {
            revert InvalidWithdrawalProof();
        }

        // Effects
        isNullifierUsed[nullifier] = true;

        emit Withdrawn(msg.sender, accountId, amount, nullifier);

        // Interactions (Safe external native transfer)
        (bool success,) = msg.sender.call{value: amount}("");
        if (!success) revert NativeTransferFailed();
    }

    /**
     * @notice Emergency administrative withdrawal during contract decommission.
     */
    function emergencyDrain(address recipient, uint256 amount) external nonReentrant onlyOwner {
        if (recipient == address(0)) revert ZeroAddress();
        if (amount > address(this).balance) revert InsufficientLiquidity(amount, address(this).balance);
        emit EmergencyDrained(recipient, amount);
        (bool success,) = recipient.call{value: amount}("");
        if (!success) revert NativeTransferFailed();
    }

    /**
     * @notice Transfer ownership (step 1 of 2).
     */
    function transferOwnership(address newOwner) external onlyOwner {
        if (newOwner == address(0)) revert ZeroAddress();
        pendingOwner = newOwner;
        emit OwnershipTransferStarted(owner, newOwner);
    }

    /**
     * @notice Accept ownership (step 2 of 2).
     */
    function acceptOwnership() external {
        if (msg.sender != pendingOwner) revert Unauthorized();
        address old = owner;
        address newOwner = pendingOwner;
        emit OwnershipTransferred(old, newOwner);
        owner = newOwner;
        pendingOwner = address(0);
    }

    /**
     * @notice Toggle emergency pause status.
     */
    function setPaused(bool _paused) external onlyOwner {
        paused = _paused;
        emit PausedStateChanged(_paused);
    }

    /// @notice Allow vault to accept plain ETH deposits.
    receive() external payable whenNotPaused {
        if (msg.value == 0) revert ZeroAmount();
        emit Deposited(msg.sender, bytes32(0), msg.value);
    }
}
