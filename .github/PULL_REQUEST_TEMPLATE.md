## Description
<!-- Provide a clear summary of the change, motivation, and relevant issue links. -->

## Component(s) Touched
- [ ] `core-c`: C11 cryptographic engine / SMT / Precompile
- [ ] `contracts`: Solidity Yul verifiers / Accumulator / Vault
- [ ] `apps/web`: Web dashboard / Vercel deployment
- [ ] `ci/cd`: GitHub Actions / Build configuration
- [ ] `docs`: Cryptographic and architectural documentation

## Checklist
- [ ] C code passes `-Wall -Wextra -Werror -pedantic -std=c11` without warnings
- [ ] AddressSanitizer and UndefinedBehaviorSanitizer pass clean (`make asan`)
- [ ] All Foundry tests pass (`forge test`) with gas report
- [ ] Web application builds cleanly (`npm run build`)
- [ ] No breaking changes to the EVM precompile ABI
- [ ] NatSpec comments updated for public Solidity functions
