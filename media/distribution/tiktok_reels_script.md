# 📱 60-Second Viral Short Script: The EVM Downcast Bug

[0:00 - 0:03] HOOK (Visual: 02_vulnerability_diff_vertical.png top red section)
"This single downcasting bug in EVM precompiles would have allowed hackers to forge Merkle proofs."

[0:03 - 0:15] PROBLEM (Visual: Zoom into the red Vulnerable Code block)
"Here's what happened: In Ethereum, every calldata word is exactly 32 bytes.
When writing our C11 Sparse Merkle Tree precompile, we needed to read the tree depth.
The developer grabbed the lowest 4 bytes into a uint32 integer."

[0:15 - 0:30] THE EXPLOIT (Visual: Animated pointer showing upper 28 bytes)
"Sounds fine, right? WRONG!
An attacker sends a depth of (1 << 64) + 8.
Because the code never checked the upper 28 bytes, the EVM silently truncated the depth to 8!
An invalid 256-depth proof suddenly gets evaluated as a valid 8-depth proof!"

[0:30 - 0:45] THE FIX (Visual: Cut to Green Patched Code box)
"The fix? Never downcast without strict zero-checks!
We added a loop verifying that all upper 28 bytes are strictly 0x00, plus a hard bound check.
Now, the precompile rejects malicious calldata immediately."

[0:45 - 0:60] CALL TO ACTION (Visual: Terminal test suite 34/34 passing)
"We pushed the entire project open source: C11 core, EVM precompile, and Solidity contracts with 34 green Foundry tests.
Link in bio to inspect the code on GitHub!"
