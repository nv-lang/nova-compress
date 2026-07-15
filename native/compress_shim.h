/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* nova-compress: thin C shim over libbrotlidec (brotli decoder) for the
 * `compress` package (extracted from Nova monorepo std/encoding/compress +
 * nova_rt/brotli_shim.*, Plan 205 Ф.1).
 *
 * PURE PROTOTYPES with no brotli dependency, so this header is safe to
 * force-include (`/FI` MSVC, `-include` clang/gcc) into every translation unit
 * that calls a compress_brotli_* extern — every CU using BrotliReader needs the
 * prototype visible in ITS OWN TU, or C falls back to implicit-int declaration
 * (32-bit) -> 64-bit handle truncation. The DEFINITIONS live in
 * compress_shim.c, which is compiled (and brotlidec.lib linked) whenever this
 * package is a build dependency (generic [ffi] pipeline, nova.toml — mirrors
 * nova-tls's native/tls_shim.h). Because brotlidec.lib is vendored WITH this
 * package (native/brotli/lib), it is always present; the link is unconditional
 * (Plan 205 path 1 — linker DCE removes the dead code for a program that never
 * decodes brotli).
 *
 * Model mirrors std/fs's fd + (buf, len) FFI shape: the caller owns Nova []u8
 * buffers and passes .ptr()+len; the shim never retains a Nova pointer across
 * calls (fed input is copied into a malloc'd accumulation buffer). Handles are
 * an opaque intptr_t carrying a (NovaBrotliDec*). All state is malloc'd (not
 * GC), released by compress_brotli_dec_free — the Nova wrapper guarantees the
 * free on every exit path.
 *
 * C-symbol convention (spec 07-modules): public shim symbols carry the package
 * prefix `compress_*`; static helpers would be `_compress_*`. `nova_*` is
 * reserved to the runtime.
 */
#ifndef NOVA_COMPRESS_SHIM_H
#define NOVA_COMPRESS_SHIM_H

#include <stdint.h>
/* Handles/counts cross as `intptr_t` — identical to nova_rt's `nova_int` (the
 * address-sized int, Plan 133). Using intptr_t keeps this header self-contained
 * so compress_shim.c compiles as a standalone TU without pulling nova_rt.h. */

/* Create a streaming decoder. Returns an opaque handle (>0), or 0 on OOM. */
intptr_t compress_brotli_dec_new(void);

/* Append `len` compressed bytes (copied internally). 0 = OK, -1 = OOM/bad-arg. */
intptr_t compress_brotli_dec_feed(intptr_t h, const uint8_t* p, intptr_t len);

/* Decode into `out` (up to `out_cap` bytes). Returns bytes written (>=0), or -1
 * on a decode error (query compress_brotli_dec_error). After the call, inspect
 * compress_brotli_dec_done / compress_brotli_dec_needs_input for stream state. */
intptr_t compress_brotli_dec_pull(intptr_t h, uint8_t* out, intptr_t out_cap);

/* 1 once the stream reached clean end (BROTLI_DECODER_RESULT_SUCCESS), else 0. */
intptr_t compress_brotli_dec_done(intptr_t h);

/* 1 if the last pull blocked needing more input (truncated when no more feeds). */
intptr_t compress_brotli_dec_needs_input(intptr_t h);

/* Detailed BrotliDecoderErrorCode (<0) after a -1 pull, else 0. */
intptr_t compress_brotli_dec_error(intptr_t h);

/* Destroy the decoder + free all internal buffers. Idempotent-safe on 0. */
void compress_brotli_dec_free(intptr_t h);

#endif /* NOVA_COMPRESS_SHIM_H */
