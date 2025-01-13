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
symbols in the code to (hopefully) make ongoing integration of upstream QBE more
straightforward.

libqbe.h then presents an externally-visible interface to the code (amalgamated
into libqbe.c). So, in order to use libqbe in your project, you need build only
libqbe.c (which does no `#include`s beyond the standard C library), and write
your front end against libqbe.h.


## Tutorial

libqbe uses `Lq`, `LQ_`, and `lq_` as prefixes for types, defines/constants, and
functions respectively.

The smallest possible usage looks like this, which you can confirm that you can
build libqbe.c correctly.
```c
#include "libqbe.h"

int main(void) {
  lq_func_start(lq_linkage_export(), lq_type_word, "main");
  lq_i_ret(lq_const_int(0));
  lq_func_end();

  lq_shutdown();
}
```

For a slightly more involved example, see `hello.c`.

## Future plans

- The output of the library is identical to the command line tool (i.e. input
  suitable for `as`. I plan to have libqbe emit other formats in the future.
  Perhaps directly to object files, rather than requiring an assembler, or
  directly into memory for JIT-style applications.
- The API currently uses a lot of implicit context (i.e. current function,
  current data definition, current block). This is because of the parsing model
  and input format of QBE, where only one top-level data or function can be
  lexically "active" at a time. It would be nice to be able to define multiple
  functions simultaneously, but the tradeoff is divergence from the original
  codebase and probably a more verbose API style.
