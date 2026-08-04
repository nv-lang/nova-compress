[English](README.md) | **Русский**

# nova-compress

Кодеки сжатия для [Nova](https://nv-lang.org) — `deflate`/`gzip`/`zlib`
(чистый Nova, RFC 1950/1951/1952), одноразовое и потоковое
кодирование/декодирование с пределом-«бомбой» (D334) против бомб
декомпрессии, плюс контрольные суммы CRC-32/Adler-32, плюс brotli
(RFC 7932) **декодирование** через завендоренный C-декодер.

Бэкенд для brotli: декодер [google/brotli](https://github.com/google/brotli),
завендорен как ИСХОДНИК в `native/brotli/` (v1.2.0, только декодер —
`dec/` + `common/`, кодировщик отброшен) и собирается тонкой C-прослойкой
(`native/compress_shim.c`) — чистый C, для сборки этого пакета Rust/cargo не
нужен. brotlidec собирает себя сам из завендоренных исходников при первом
`nova test`/`nova build` через общий механизм сборки-с-кэшированием
`[ffi] vendor_src_dirs` (см. `native/brotli/VENDORED.md`) — ручная установка
через vcpkg/system не нужна, и предсобранный бинарник в репозиторий не
закоммичен (вручную положенная предсобранная библиотека под
`native/brotli/lib/` тоже работает как есть, минуя vendor-сборку). Всё
остальное (deflate, gzip, zlib, контрольные суммы) — чистый Nova, вообще без
нативных зависимостей.

Извлечено из монорепозитория Nova, из `std/encoding/compress`, в отдельный
репозиторий по
[плану 205](https://github.com/nv-lang/nova/blob/main/docs/plans/205-compress-out-of-nova-rt.md),
следуя паттерну нативных модулей, заложенному nova-tls для mbedTLS
([план 193](https://github.com/nv-lang/nova/blob/main/docs/plans/193-nova-tls-repo.md)):
фасад `.nv` + C-прослойка + завендоренный C-исходник, подключено через
`[ffi]`, без Rust. Публичный API не изменился по сравнению с
`std.encoding.compress` — сдвинулся только путь модуля
(`std.encoding.compress.*` -> `compress.*`).

## Использование

```nova
import compress.{gzip_decode, gzip_encode, brotli_decode}

fn round_trip() -> Result[(), CompressError] {
    ro raw = "hello, nova".bytes()
    ro packed = gzip_encode(raw, level: 6)!!
    ro back = gzip_decode(packed, max_output: 1 << 20)!!  // bomb-cap
    back
}
```

## Структура

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

## Автономная сборка

Нужен тулчейн Nova (CLI `nova` + clang). Rust/cargo не нужен, ручная
установка brotli не нужна — brotlidec собирает себя сам из завендоренных
исходников в `native/brotli/` при первом запуске (положите предсобранную
библиотеку `brotlidec` под `native/brotli/lib/` вместо этого, если хотите
пропустить эту разовую сборку).

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

## Лицензия

Двойная лицензия — [MIT](LICENSE-MIT) или [Apache-2.0](LICENSE-APACHE), на
ваш выбор — те же условия, что у компилятора Nova и стандартной библиотеки.
Завендоренный исходник brotli под `native/brotli/` несёт собственную
апстримную MIT-лицензию (`native/brotli/LICENSE`).
