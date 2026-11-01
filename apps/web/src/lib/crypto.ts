/**
 * Aegis Cryptographic Engine (Web / Client-Side)
 * Pure TypeScript implementation of Ethereum Keccak-256 and Sparse Merkle Tree Verification.
 */

// Keccak-f[1600] Round Constants
const RC = [
  0x0000000000000001n, 0x0000000000008082n, 0x800000000000808an,
  0x8000000080008000n, 0x000000000000808bn, 0x0000000080000001n,
  0x8000000080008081n, 0x8000000000008009n, 0x000000000000008an,
  0x0000000000000088n, 0x0000000080008009n, 0x000000008000000an,
  0x000000008000808bn, 0x800000000000008bn, 0x8000000000008089n,
  0x8000000000008003n, 0x8000000000008002n, 0x8000000000000080n,
  0x000000000000800an, 0x800000008000000an, 0x8000000080008081n,
  0x8000000000008080n, 0x0000000080000001n, 0x8000000080008008n,
];

const RHO = [
  0n, 1n, 62n, 28n, 27n, 36n, 44n, 6n, 55n, 20n, 3n, 10n, 43n, 25n, 39n,
  41n, 45n, 15n, 21n, 8n, 18n, 2n, 61n, 56n, 14n,
];

const PI = [
  0, 10, 20, 5, 15, 16, 1, 11, 21, 6, 7, 17, 2, 12, 22, 23, 8, 18, 3, 13,
  14, 24, 9, 19, 4,
];

function rotl64(x: bigint, n: bigint): bigint {
  const shift = n % 64n;
  return ((x << shift) | (x >> (64n - shift))) & 0xffffffffffffffffn;
}

function keccakF1600(state: bigint[]) {
  for (let round = 0; round < 24; round++) {
    // Theta
    const C = new Array<bigint>(5);
    for (let x = 0; x < 5; x++) {
      C[x] = state[x] ^ state[x + 5] ^ state[x + 10] ^ state[x + 15] ^ state[x + 20];
    }
    const D = new Array<bigint>(5);
    for (let x = 0; x < 5; x++) {
      D[x] = C[(x + 4) % 5] ^ rotl64(C[(x + 1) % 5], 1n);
    }
    for (let i = 0; i < 25; i++) {
      state[i] ^= D[i % 5];
    }

    // Rho and Pi
    const B = new Array<bigint>(25);
    for (let i = 0; i < 25; i++) {
      B[PI[i]] = rotl64(state[i], RHO[i]);
    }

    // Chi
    for (let y = 0; y < 5; y++) {
      const y5 = y * 5;
      for (let x = 0; x < 5; x++) {
        state[y5 + x] =
          B[y5 + x] ^ ((~B[y5 + ((x + 1) % 5)]) & B[y5 + ((x + 2) % 5)]);
      }
    }

    // Iota
    state[0] ^= RC[round];
  }
}

export function keccak256(msg: Uint8Array): Uint8Array {
  const rateBytes = 136;
  const state = new Array<bigint>(25).fill(0n);

  let offset = 0;
  const len = msg.length;

  while (len - offset >= rateBytes) {
    const view = new DataView(msg.buffer, msg.byteOffset + offset, rateBytes);
    for (let i = 0; i < 17; i++) {
      state[i] ^= view.getBigUint64(i * 8, true);
    }
    keccakF1600(state);
    offset += rateBytes;
  }

  // Final block with Ethereum padding (0x01 ... 0x80)
  const remaining = len - offset;
  const block = new Uint8Array(rateBytes);
  block.set(msg.subarray(offset));
  block[remaining] ^= 0x01;
  block[rateBytes - 1] ^= 0x80;

  const view = new DataView(block.buffer, block.byteOffset, rateBytes);
  for (let i = 0; i < 17; i++) {
    state[i] ^= view.getBigUint64(i * 8, true);
  }
  keccakF1600(state);

  const out = new Uint8Array(32);
  const outView = new DataView(out.buffer);
  for (let i = 0; i < 4; i++) {
    outView.setBigUint64(i * 8, state[i], true);
  }
  return out;
}

export function hexToBytes(hex: string): Uint8Array {
  const clean = hex.startsWith('0x') ? hex.slice(2) : hex;
  const bytes = new Uint8Array(clean.length / 2);
  for (let i = 0; i < bytes.length; i++) {
    bytes[i] = parseInt(clean.substring(i * 2, i * 2 + 2), 16);
  }
  return bytes;
}

export function bytesToHex(bytes: Uint8Array): string {
  let hex = '0x';
  for (let i = 0; i < bytes.length; i++) {
    hex += bytes[i].toString(16).padStart(2, '0');
  }
  return hex;
}

export function hashNodes(left: Uint8Array, right: Uint8Array): Uint8Array {
  const combined = new Uint8Array(64);
  combined.set(left, 0);
  combined.set(right, 32);
  return keccak256(combined);
}

export function computeLeafHash(key: Uint8Array, value: Uint8Array): Uint8Array {
  let isZero = true;
  for (let i = 0; i < value.length; i++) {
    if (value[i] !== 0) { isZero = false; break; }
  }
  if (isZero) return new Uint8Array(32);

  const combined = new Uint8Array(64);
  combined.set(key, 0);
  combined.set(value, 32);
  return keccak256(combined);
}

export function getKeyBit(key: Uint8Array, bitIdx: number): number {
  if (bitIdx >= 256) return 0;
  const byteIdx = Math.floor(bitIdx / 8);
  const bitPos = 7 - (bitIdx % 8);
  return (key[byteIdx] >> bitPos) & 1;
}

export function verifyProof(
  rootHex: string,
  keyHex: string,
  valueHex: string,
  siblingHexes: string[]
): { valid: boolean; computedRoot: string } {
  const root = hexToBytes(rootHex);
  const key = hexToBytes(keyHex);
  const value = hexToBytes(valueHex);
  const siblings = siblingHexes.map(hexToBytes);

  let current = computeLeafHash(key, value);
  const depth = siblings.length;

  for (let i = depth - 1; i >= 0; i--) {
    const bit = getKeyBit(key, i);
    const sib = siblings[i];
    if (bit === 0) {
      current = hashNodes(current, sib);
    } else {
      current = hashNodes(sib, current);
    }
  }

  const computedRootHex = bytesToHex(current);
  const valid = (computedRootHex.toLowerCase() === bytesToHex(root).toLowerCase());
  return { valid, computedRoot: computedRootHex };
}

export function encodeSolidityCalldata(
  rootHex: string,
  keyHex: string,
  valueHex: string,
  siblingHexes: string[]
): string {
  // Selector: verifyProof(bytes32,bytes32,bytes32,bytes32[]) -> 0x532810b1
  let calldata = '0x532810b1';

  // Format 32-byte hex without 0x prefix
  const cleanHex = (h: string) => (h.startsWith('0x') ? h.slice(2) : h).padStart(64, '0');

  calldata += cleanHex(rootHex);
  calldata += cleanHex(keyHex);
  calldata += cleanHex(valueHex);
  // Offset to dynamic array (128 bytes = 0x80)
  calldata += '0000000000000000000000000000000000000000000000000000000000000080';
  // Array length
  calldata += siblingHexes.length.toString(16).padStart(64, '0');
  // Sibling items
  for (const sib of siblingHexes) {
    calldata += cleanHex(sib);
  }
  return calldata;
}
