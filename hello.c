#include "libqbe.h"

/*
function w $add(w %a, w %b) {              # Define a function add
@start
	%c =w add %a, %b                   # Adds the 2 arguments
	ret %c                             # Return the result
}
export function w $main() {                # Main function
@start
	%r =w call $add(w 1, w 1)          # Call add(1, 1)
	call $printf(l $fmt, ..., w %r)    # Show the result
	ret 0
}
data $fmt = { b "One and one make %d!\n", b 0 }
*/

void compile(void) {
  lq_func_start(lq_linkage_default, lq_type_word, "add");
  LqRef a = lq_func_param(lq_type_word);
  LqRef b = lq_func_param(lq_type_word);
  LqRef c = lq_i_add(lq_type_word, a, b);
  lq_i_ret(c);
  LqRef add_func = lq_func_end();
  (void)add_func;

#if 0
  lq_data_start(lq_linkage_default, "fmt");
  lq_data_string("One and one make %d!\n");
  lq_data_byte(0);
  LqRef fmt = lq_data_end();
#endif

  lq_func_start(lq_linkage_export, lq_type_word, "main");
  LqRef r = lq_i_call2(lq_type_word, add_func, lq_type_word, lq_const_int(1), lq_type_word,
                       lq_const_int(1));
  (void)r;
  //LqRef printf_func = lq_extern("printf");
  //lq_i_call_varargs(printf, r);
  lq_i_ret(lq_const_int(0));
  LqRef main = lq_func_end();
  (void)main;
}

int main(void) {
  printf("---------- COMPILED FOR DEFAULT TARGET FOLLOWS ----------\n");
  lq_init(LQ_TARGET_DEFAULT, stdout, "P");
  compile();
  lq_shutdown();

  printf("---------- COMPILED FOR AMD64_WIN FOLLOWS  ----------\n");
  lq_init(LQ_TARGET_AMD64_WINDOWS, stdout, "");
  compile();
  lq_shutdown();
}
