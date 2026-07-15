# Vendored brotli (decoder-only)

Upstream: https://github.com/google/brotli
Version: v1.2.0 (tag), commit `028fb5a23661f123017c060daa546b55cf4bde29`
License: MIT (see `LICENSE`)

Only the decoder + shared common code is vendored — `c/enc/` (the encoder)
is dropped entirely, this package never compresses brotli, only decodes it:

- `dec/` <- upstream `c/dec/*.c` + `*.h` (bit_reader, decode, huffman, prefix,
  prefix_inc, state, static_init — full contents of upstream's `c/dec/`).
- `common/` <- upstream `c/common/*.c` + `*.h` EXCEPT `dictionary.bin` /
  `dictionary.bin.br` (raw dictionary source assets used only by upstream's
  own generator to produce `dictionary_inc.h` — that generated header is
  vendored as-is and is the only dictionary input the build needs).
- `include/brotli/` <- upstream `c/include/brotli/{decode,port,
  shared_dictionary,types}.h` (public decoder-facing headers only;
  `encode.h` dropped along with the encoder).

Build: driven generically by `[ffi] vendor_src_dirs` in `nova.toml` — see
that file's comments and `compiler-codegen/src/test_runner.rs`
(`build_missing_vendor_ffi_libs`) in the main `nova` repo (same mechanism
nova-tls uses for mbedTLS — Plan 193 Ф.2 gate-3 "195-pattern"). First
`nova test` (or any build consuming this package's `[ffi]`) compiles every
`.c` directly under `dec/` and `common/` and archives the result into
`native/brotli/lib/` (gitignored, machine/toolchain-specific) under the
single declared `[ffi] libs` name, `brotlidec`. Cached — rebuilt only if
that archive is missing.

To bump the vendored version: clone a newer tag, re-copy `c/dec/`, `c/common/`
(minus the two `dictionary.bin*` assets) and the four public headers above,
replace `LICENSE`, update this file + `version.txt`, delete any stale built
archive under `native/brotli/lib/` — no content-hash invalidation, manual bump.
