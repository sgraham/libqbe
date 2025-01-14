#include "libqbe.h"

/*

compile() implements the equivalent of this example from the QBE documentation.

----------

function w $add(w %a, w %b) {          # Define a function add
@start
    %c =w add %a, %b                   # Adds the 2 arguments
    ret %c                             # Return the result
}

data $fmt = { b "One and one make %d!\n", b 0 }

export function w $main() {            # Main function
@start
    %r =w call $add(w 1, w 1)          # Call add(1, 1)
    call $printf(l $fmt, ..., w %r)    # Show the result
    ret 0
}

----------

By default, there are fewer strings in the API version, for example, the
function arguments don't necessarily need a name, instead you can just use the
LqRef handle to refer to them. You can provide names though by using
lq_func_param_named() (and similar _named suffix functions) so that you can see
helpful names when viewing debug output from QBE.

Be aware that the lifetime of LqRef is only the duration of the function in
which they're defined. In particular, it "looks" like you could cache the result
of lq_ref_for_symbol() rather than calling it when inside the range of
lq_func_start() and lq_func_end(), but you cannot.

*/
void compile(void) {
  lq_func_start(lq_linkage_default, lq_type_word, "add");  // Define a function add.
  LqRef a = lq_func_param(lq_type_word);
  LqRef b = lq_func_param(lq_type_word);

  LqRef c = lq_i_add(lq_type_word, a, b);  // Adds the 2 arguments.
  lq_i_ret(c);  // Return the result.
  LqSymbol add_func = lq_func_end();  // Get a symbol for the created function.

  // Create a data item for the string we're going to pass to printf.
  lq_data_start(lq_linkage_default, "fmt");
  lq_data_string("One and one make %d!\n");
  lq_data_byte(0);
  LqSymbol fmt = lq_data_end();

  lq_func_start(lq_linkage_export, lq_type_word, "main");  // Main function.
  LqRef r = lq_i_call(lq_type_word, lq_ref_for_symbol(add_func),
                      lq_type_word, lq_const_int(1),
                      lq_type_word, lq_const_int(1));  // Call add(1, 1).

  LqRef printf_func = lq_extern("printf");
  lq_i_call_varargs(lq_type_word, printf_func,
                    lq_type_long, lq_ref_for_symbol(fmt),
                    lq_type_word, r);  // Show the result with printf.

  lq_i_ret(lq_const_int(0));
  lq_func_end();
}

int main(void) {
  lq_init(LQ_TARGET_DEFAULT, stdout, "");
  compile();
  lq_shutdown();
}
