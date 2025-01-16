# libqbe

## Rationale

This is a "packaging" of [QBE](https://c9x.me/compile/).

QBE is designed to take a text input format, and emit text assembler. This is
very convenient and debuggable! But in profiling my front-end on large input
files, I found that a majority of the run time was in string manipulation to
generate the input file to be passed to QBE.

libqbe attempts to improve this by constructing the same data structures in
memory that the QBE parser does from input text, and then runs the same backend
processing.

The QBE source code is written in a terse style, that is, short identifiers
often with global scope. This is sensible for a standalone binary tool, but in
order to make the code suitable for linking into other C programs without
polluting the global namespace, it's preprocessed by `build_amalg.py` (a bunch
of janky regex hacks in a trench coat) to rename and static-ize symbols. A
script to amalgamate-and-rename is used instead of just directly renaming
symbols in the code and carrying a set of patches to (hopefully) make ongoing
integration of upstream QBE more straightforward.

`libqbe.h` then presents an externally-visible interface to the code
(amalgamated into `libqbe.c`). So, in order to use libqbe in your project, you
need build only libqbe.c (which does no `#include`s beyond the standard C
library), and write your front end against `libqbe.h`.

## Releases

There's no formal versioned releases, but an amalgamated copy of `libqbe.c` and
`libqbe.h` cut from the head of trunk [can be
downloaded](https://github.com/sgraham/libqbe/releases/tag/nightly).

## Warning

**It is quite possible that I have introduced bugs in this packaging, as the
integration with the existing parser is a bit subtle. If you use this library,
and encounter a bug in code generation, please be sure to reproduce the bug in
"plain" QBE before engaging the QBE mailing list.** You can use the
`debug_flags` of `lq_init()` to emit textual QBE input that can be fed to the
standard command line tool if necessary.

## Tutorial

libqbe uses `Lq`, `LQ_`, and `lq_` as prefixes for types, defines/constants, and
functions respectively.

The smallest possible usage looks like this, which you can use to confirm that
you can build `libqbe.c` correctly.

```c
#include "libqbe.h"

int main(void) {
  lq_init(LQ_TARGET_DEFAULT, stdout, "");

  lq_func_start(lq_linkage_export, lq_type_word, "main");
  lq_i_ret(lq_const_int(0));
  lq_func_end();

  lq_shutdown();
}
```

For a slightly more involved example, see the examples in  `hello.c` and
`more.c`.

## Future plans

TODOs (filling out API, etc.) can be found under
[Issues](https://github.com/sgraham/libqbe/issues).

Longer term/larger issues:

- The output of the library is identical to the command line tool (i.e. input
  suitable for `as`. It might be nice to have libqbe emit other formats in the
  future. Perhaps directly to object files, rather than requiring an assembler,
  or directly into memory for JIT-style applications.
- The API currently uses a lot of implicit context (i.e. current function,
  current data definition, current block, current type). This is because of the
  parsing model and input format of QBE, where only one top-level data or
  function can naturally be lexically "active" at a time. It would be nice to be
  able to define multiple functions simultaneously, but the tradeoff would be
  divergence from the original codebase and probably a more verbose API style.
