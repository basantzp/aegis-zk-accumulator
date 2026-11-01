// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

/**
 * @title IAegisVault
 * @notice Production Settlement and Cryptographic Vault Interface.
 * @author Aegis Project Contributors
 */
interface IAegisVault {
    /**
     * @notice Emitted on deposit of native ETH.
     * @param depositor Address of the funding sender.
     * @param accountId Off-chain 256-bit account ID or public key hash.
     * @param amount ETH amount deposited in wei.
     */
    event Deposited(address indexed depositor, bytes32 indexed accountId, uint256 amount);

    /**
     * @notice Emitted upon successful cryptographic withdrawal proof verification.
     * @param recipient Target destination address.
     * @param accountId Off-chain 256-bit account ID.
     * @param amount Amount transferred in wei.
     * @param nullifier Cryptographic nullifier consumed to prevent double-spending.
     */
    event Withdrawn(
        address indexed recipient,
        bytes32 indexed accountId,
        uint256 amount,
        bytes32 indexed nullifier
    );

    /**
     * @notice Deposit ETH into the vault for a designated off-chain account.
     * @param accountId Off-chain account identifier.
     */
    function deposit(bytes32 accountId) external payable;

    /**
     * @notice Execute a trustless withdrawal backed by a Merkle inclusion proof.
     * @param root Valid historical state root in the accumulator.
     * @param accountId Off-chain account ID.
     * @param amount Withdrawal amount.
     * @param nullifier Unique random nullifier.
     * @param proof Sibling Merkle path.
     */
    function withdraw(
        bytes32 root,
        bytes32 accountId,
        uint256 amount,
        bytes32 nullifier,
        bytes32[] calldata proof
    ) external;

    /**
     * @notice Check if a nullifier has already been consumed.
     */
    function isNullifierUsed(bytes32 nullifier) external view returns (bool);
}
