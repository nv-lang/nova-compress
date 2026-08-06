# nova-compress

Compression codecs for [Nova](https://nv-lang.org) — `deflate`/`gzip`/`zlib`
(pure Nova, RFC 1950/1951/1952) one-shot and streaming encode/decode with a
bomb-cap (D334) against decompression bombs, plus CRC-32/Adler-32 checksums,
plus brotli (RFC 7932) **decode** over a vendored C decoder.

Backend for brotli: [google/brotli](https://github.com/google/brotli)'s
decoder, vendored as SOURCE at `native/brotli/` (v1.2.0, decoder-only —
`dec/` + `common/`, the encoder is dropped) and compiled by a thin C shim
(`native/compress_shim.c`) — pure C, no Rust/cargo required to build this
package. brotlidec self-builds from the vendored source on first
`nova test`/`nova build` via the generic `[ffi] vendor_src_dirs`
build-and-cache mechanism (see `native/brotli/VENDORED.md`) — no manual
vcpkg/system install step and no prebuilt binary checked into the repo (a
manually dropped-in prebuilt lib under `native/brotli/lib/` still works too
and is used as-is, skipping the vendor build). Everything else (deflate,
gzip, zlib, checksums) is pure Nova — no native dependency at all.

Extracted from the Nova monorepo's `std/encoding/compress` into a standalone
repository per
[Plan 205](https://github.com/nv-lang/nova/blob/main/docs/plans/205-compress-out-of-nova-rt.md),
following the native-module pattern nova-tls established for mbedTLS
([Plan 193](https://github.com/nv-lang/nova/blob/main/docs/plans/193-nova-tls-repo.md)):
`.nv` facade + `.c` shim + vendored C source, wired through `[ffi]`, zero
Rust. Public API is unchanged from `std.encoding.compress` — only the module
path moved (`std.encoding.compress.*` -> `compress.*`).

## Usage

```nova
import compress.{gzip_decode, gzip_encode, brotli_decode}

fn round_trip() -> Result[(), CompressError] {
    ro raw = "hello, nova".bytes()
    ro packed = gzip_encode(raw, level: 6)!
    ro back = gzip_decode(packed, max_output: 1 << 20)!  // bomb-cap
    back
}
```

## Layout

```
nova-compress/
├── nova.toml              [package] name = "compress"; [lib] src = "src"; [ffi] native shim
├── native/
│   ├── compress_shim.c      libbrotlidec backend (compiled via [ffi] c_shims)
│   ├── compress_shim.h       C-side prototypes (Nova <-> C ABI contract)
│   └── brotli/                vendored brotli v1.2.0 SOURCE (decoder-only)
│       ├── dec/                  upstream c/dec/*.{c,h}
│       ├── common/                upstream c/common/*.{c,h} (minus dictionary.bin*)
│       ├── include/brotli/         public decoder-facing headers
│       ├── LICENSE                 brotli's own MIT license
│       └── VENDORED.md             vendoring provenance + rebuild/bump notes
└── src/
    ├── ffi.nv                extern "C" fn declarations against native/compress_shim.c
    ├── error.nv                CompressError (typed error surface)
    ├── deflate.nv / inflate.nv  pure-Nova DEFLATE (RFC 1951) encode/decode
    ├── gzip.nv / zlib.nv        pure-Nova framing (RFC 1952 / RFC 1950) over deflate/inflate
    ├── checksum.nv              CRC-32 / Adler-32
    ├── brotli.nv                brotli (RFC 7932) decode over the C shim
    ├── *_test.nv                peer tests (same-module, positive)
    └── neg/                     EXPECT_COMPILE_ERROR fixtures (standalone CUs)
```

## Building standalone

Requires the Nova toolchain (`nova` CLI + clang). No Rust/cargo, no manual
brotli install — brotlidec self-builds from the vendored source at
`native/brotli/` on first run (drop a prebuilt `brotlidec` lib under
`native/brotli/lib/` instead if you'd rather skip that one-time build).

```sh
# Boehm GC (mandatory Nova runtime dep) needs its own lib/include dirs —
# point NOVA_GC_LIB_DIR (+ optional NOVA_GC_INCLUDE_DIR) at a prebuilt
# bdwgc if it isn't reachable via the default vcpkg/system lookup.
export NOVA_GC_LIB_DIR=/path/to/vcpkg_installed/x64-windows-static/lib
export NOVA_GC_INCLUDE_DIR=/path/to/vcpkg_installed/x64-windows-static/include

# `nova` does not (yet) bundle/locate the standard library relative to the
# nova.exe install — a standalone package must point it at a Nova checkout's
# std/ via NOVA_STD_PATH:
export NOVA_STD_PATH=/path/to/nova/std

# Ditto for the compiler's own C runtime (compiler-codegen/nova_rt/ + the
# libuv submodule it needs):
export NOVA_CG_INCLUDE=/path/to/nova/compiler-codegen
export NOVA_RT_DIR=/path/to/nova/compiler-codegen/nova_rt

nova test src
```

## License

Dual-licensed under [MIT](LICENSE-MIT) or [Apache-2.0](LICENSE-APACHE), at
your option — same terms as the Nova compiler and standard library. The
vendored brotli source under `native/brotli/` carries its own upstream MIT
license (`native/brotli/LICENSE`).
