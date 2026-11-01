// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import {Script, console} from "forge-std/Script.sol";
import {AegisAccumulator} from "../src/AegisAccumulator.sol";
import {AegisVault} from "../src/AegisVault.sol";

contract DeployAegisScript is Script {
    function run() external returns (AegisAccumulator accumulator, AegisVault vault) {
        uint256 deployerPrivateKey = vm.envOr(
            "PRIVATE_KEY",
            uint256(0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80) // Anvil default key #0
        );

        address deployer = vm.addr(deployerPrivateKey);
        console.log("Deploying Aegis ZK Accumulator suite with deployer:", deployer);

        bytes32 genesisRoot = vm.envOr(
            "GENESIS_ROOT",
            bytes32(0xf1c8fc886a1b8973df719e3aa74a939e85449cdb6f45f2d8badd3af6ad2beaaf)
        );

        address sequencer = vm.envOr("SEQUENCER_ADDRESS", deployer);

        vm.startBroadcast(deployerPrivateKey);

        accumulator = new AegisAccumulator(genesisRoot, sequencer);
        console.log("AegisAccumulator deployed at:", address(accumulator));

        vault = new AegisVault(address(accumulator));
        console.log("AegisVault deployed at:", address(vault));

        vm.stopBroadcast();
    }
}
