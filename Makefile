.PHONY: all clean test bench asan build-c build-contracts build-web lint help

all: build-c build-contracts build-web
	@echo "=================================================="
	@echo "    Aegis ZK Accumulator: Build Complete!         "
	@echo "=================================================="

build-c:
	@echo "--> Building C11 Cryptographic Engine & CLI..."
	$(MAKE) -C core-c all

build-contracts:
	@echo "--> Compiling Solidity Smart Contracts..."
	cd contracts && forge build

build-web:
	@echo "--> Building Production Web Dashboard (Vercel bundle)..."
	cd apps/web && npm run build

test: test-c test-contracts
	@echo "--> All test suites passed!"

test-c:
	@echo "--> Running C Unit Tests..."
	$(MAKE) -C core-c test

test-contracts:
	@echo "--> Running Foundry Solidity Tests & Fuzzing..."
	cd contracts && forge test

asan:
	@echo "--> Running AddressSanitizer & UndefinedBehaviorSanitizer..."
	$(MAKE) -C core-c asan

bench:
	@echo "--> Running C Microbenchmarks..."
	$(MAKE) -C core-c bench
	@echo "--> Running Solidity Gas Benchmarks..."
	cd contracts && forge test --gas-report

clean:
	@echo "--> Cleaning build artifacts..."
	$(MAKE) -C core-c clean
	rm -rf contracts/out contracts/cache
	rm -rf apps/web/dist apps/web/node_modules/.vite

fmt:
	@echo "--> Formatting Solidity contracts and C source code..."
	cd contracts && forge fmt
	clang-format -i core-c/include/*.h core-c/src/*.c core-c/cli/*.c core-c/tests/*.c

help:
	@echo "Available Makefile targets:"
	@echo "  all             Build C library, Solidity contracts, and Web client"
	@echo "  build-c         Compile core-c libraries and aegis-cli"
	@echo "  build-contracts Compile Solidity smart contracts via forge"
	@echo "  build-web       Bundle Web application for Vercel deployment"
	@echo "  test            Run both C and Solidity test suites"
	@echo "  test-c          Run C unit tests"
	@echo "  test-contracts  Run Foundry unit & fuzz tests"
	@echo "  asan            Run C test suite under AddressSanitizer"
	@echo "  bench           Execute cryptographic benchmarks & gas snapshots"
	@echo "  clean           Remove all generated binaries and cache"
