// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import {IAegisAccumulator} from "./interfaces/IAegisAccumulator.sol";
import {AegisCryptoLib} from "./libraries/AegisCryptoLib.sol";

/**
 * @title AegisAccumulator
 * @notice Production-Grade Cryptographic State Accumulator for Rollups and Stateless Verifiers.
 * @author Aegis Project Contributors
 */
contract AegisAccumulator is IAegisAccumulator {
    using AegisCryptoLib for bytes32;

    /// @notice Maximum number of historical roots maintained in the ring buffer.
    uint256 public constant ROOT_HISTORY_CAPACITY = 256;

    /// @notice Address of the contract owner (admin governance).
    address public owner;

    /// @notice Pending owner for safe two-step ownership transfers.
    address public pendingOwner;

    /// @notice Authorized sequencer authorized to commit state root updates.
    address public sequencer;

    /// @notice Latest verified state root.
    bytes32 public override currentRoot;

    /// @notice Monotonically increasing batch sequence index.
    uint256 public override currentBatchId;

    /// @notice Emergency circuit breaker pause status.
    bool public paused;

    /// @notice Circular ring buffer of historical state roots.
    bytes32[ROOT_HISTORY_CAPACITY] private _historicalRoots;

    /// @notice Maps root hash to block number when it was committed (0 if uncommitted).
    mapping(bytes32 => uint256) public rootCommittedBlock;

    /// @dev Events for administrative transitions.
    event SequencerUpdated(address indexed previousSequencer, address indexed newSequencer);
    event OwnershipTransferStarted(address indexed previousOwner, address indexed newOwner);
    event OwnershipTransferred(address indexed previousOwner, address indexed newOwner);
    event PausedStateChanged(bool isPaused);

    /// @dev Custom errors for gas efficiency.
    error Unauthorized();
    error ContractPaused();
    error RootMismatch(bytes32 expected, bytes32 actual);
    error InvalidRoot();
    error InvalidBatchDimensions();
    error BatchVerificationFailed(uint256 index);
    error ZeroAddress();

    modifier onlyOwner() {
        if (msg.sender != owner) revert Unauthorized();
        _;
    }

    modifier onlySequencerOrOwner() {
        if (msg.sender != sequencer && msg.sender != owner) revert Unauthorized();
        _;
    }

    modifier whenNotPaused() {
        if (paused) revert ContractPaused();
        _;
    }

    /**
     * @notice Construct a new AegisAccumulator.
     * @param initialRoot Genesis or initial state root.
     * @param initialSequencer Sequencer authorized for batch submissions.
     */
    constructor(bytes32 initialRoot, address initialSequencer) {
        if (initialSequencer == address(0)) revert ZeroAddress();

        owner = msg.sender;
        sequencer = initialSequencer;
        currentRoot = initialRoot;
        rootCommittedBlock[initialRoot] = block.number;
        _historicalRoots[0] = initialRoot;
        currentBatchId = 1;

        emit StateRootUpdated(bytes32(0), initialRoot, 0, block.timestamp);
    }

    /**
     * @inheritdoc IAegisAccumulator
     */
    function verifyProof(
        bytes32 root,
        bytes32 key,
        bytes32 value,
        bytes32[] calldata siblings
    ) external pure override returns (bool isValid) {
        return AegisCryptoLib.verifyProofCalldata(root, key, value, siblings);
    }

    /**
     * @inheritdoc IAegisAccumulator
     */
    function verifyBatchProof(
        bytes32 root,
        bytes32[] calldata keys,
        bytes32[] calldata values,
        bytes32[][] calldata siblings
    ) external pure override returns (bool allValid) {
        uint256 count = keys.length;
        if (count != values.length || count != siblings.length) {
            revert InvalidBatchDimensions();
        }

        for (uint256 i = 0; i < count; ) {
            if (!AegisCryptoLib.verifyProofCalldata(root, keys[i], values[i], siblings[i])) {
                return false;
            }
            unchecked { ++i; }
        }

        return true;
    }

    /**
     * @notice Commit a new state root with optional cryptographic transition verification.
     * @param expectedOldRoot The current root that must match on-chain state to prevent reorgs.
     * @param newRoot The new state root to commit.
     * @param transitionKeys Keys verified as transitioning in this batch.
     * @param transitionValues Old values verified as existing in expectedOldRoot.
     * @param transitionProofs Merkle proofs proving existence in expectedOldRoot.
     */
    function updateRoot(
        bytes32 expectedOldRoot,
        bytes32 newRoot,
        bytes32[] calldata transitionKeys,
        bytes32[] calldata transitionValues,
        bytes32[][] calldata transitionProofs
    ) external onlySequencerOrOwner whenNotPaused {
        if (expectedOldRoot != currentRoot) {
            revert RootMismatch(expectedOldRoot, currentRoot);
        }
        if (newRoot == bytes32(0)) {
            revert InvalidRoot();
        }

        // Verify state transition proofs against old root if provided
        uint256 numProofs = transitionKeys.length;
        if (numProofs > 0) {
            if (numProofs != transitionValues.length || numProofs != transitionProofs.length) {
                revert InvalidBatchDimensions();
            }

            for (uint256 i = 0; i < numProofs; ) {
                if (!AegisCryptoLib.verifyProofCalldata(expectedOldRoot, transitionKeys[i], transitionValues[i], transitionProofs[i])) {
                    revert BatchVerificationFailed(i);
                }
                unchecked { ++i; }
            }
        }

        // Commit new root to ring buffer
        uint256 batchId = currentBatchId;
        _historicalRoots[batchId % ROOT_HISTORY_CAPACITY] = newRoot;
        rootCommittedBlock[newRoot] = block.number;
        currentRoot = newRoot;
        currentBatchId = batchId + 1;

        emit StateRootUpdated(expectedOldRoot, newRoot, batchId, block.timestamp);
    }

    /**
     * @inheritdoc IAegisAccumulator
     */
    function isKnownRoot(bytes32 root) external view override returns (bool) {
        if (root == bytes32(0)) return false;
        return rootCommittedBlock[root] > 0;
    }

    /**
     * @notice Retrieve an ancient root from the ring buffer.
     * @param index Slot index in ring buffer (0 to 255).
     */
    function getHistoricalRootAt(uint256 index) external view returns (bytes32) {
        if (index >= ROOT_HISTORY_CAPACITY) revert InvalidBatchDimensions();
        return _historicalRoots[index];
    }

    /* ================================================================
                            ADMINISTRATION
       ================================================================ */

    function setSequencer(address newSequencer) external onlyOwner {
        if (newSequencer == address(0)) revert ZeroAddress();
        address old = sequencer;
        sequencer = newSequencer;
        emit SequencerUpdated(old, newSequencer);
    }

    function transferOwnership(address newOwner) external onlyOwner {
        if (newOwner == address(0)) revert ZeroAddress();
        pendingOwner = newOwner;
        emit OwnershipTransferStarted(owner, newOwner);
    }

    function acceptOwnership() external {
        if (msg.sender != pendingOwner) revert Unauthorized();
        address old = owner;
        owner = pendingOwner;
        pendingOwner = address(0);
        emit OwnershipTransferred(old, owner);
    }

    function setPaused(bool isPaused) external onlyOwner {
        paused = isPaused;
        emit PausedStateChanged(isPaused);
    }
}
