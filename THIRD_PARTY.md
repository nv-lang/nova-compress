# Third-party components vendored in `nova-compress`

This package ships third-party sources inside its own tree. Vendoring means we
**redistribute** those sources, which carries obligations that linking against a
system package does not — so they are listed here explicitly rather than left to
whoever thinks to look inside `native/`.

## Brotli (decoder only)

- **Component**: the Brotli decoder behind this package's `brotli` decode path;
  compiled from source by the generic `[ffi] vendor_src_dirs` build-and-cache
  mechanism (see `nova.toml`).
- **Vendored at**: `native/brotli/{dec,common,include}/`, version **v1.2.0**
  (commit `028fb5a23661f123017c060daa546b55cf4bde29`) — see
  `native/brotli/VENDORED.md`.
- **License**: **MIT**.
- **Full text**: `native/brotli/LICENSE` (kept verbatim in the tree).
- **Source**: https://github.com/google/brotli
- **Copyright**: 2009, 2010, 2013-2016 by the Brotli Authors.

Only the decoder is vendored; the encoder is not part of this package. Nothing
in `nova-compress` modifies Brotli sources — the C shim
(`native/compress_shim.c`) only calls its public API.

The deflate/gzip/zlib codecs in this package are **not** third-party: they are
pure Nova, written for this repository.

## Relationship to this package's own licence

`nova-compress` itself is **MIT OR Apache-2.0** (see `LICENSE-MIT` /
`LICENSE-APACHE` and the `license` field in `nova.toml`), which is compatible
with Brotli's MIT terms.
