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

int main(void) {
  lq_init(LQ_TARGET_DEFAULT, "");

  lq_start_func(lq_linkage_default(), lq_type_word(), "add");
  LqRef a = lq_func_param(lq_type_word());
  Lqref b = lq_func_param(lq_type_word());
  lq_start_block();
  LqRef c = lq_emit(LQ_TYPE_WORD, LQ_ADD, a, b);  // TODO: w context
  lq_emit(LQ_RET, c);
  LqRef add_func = lq_end_func();

  lq_start_data(lq_linkage_default(), "fmt");
  lq_data_string("One and one make %d!\n");
  lq_data_byte(0);
  LqRef fmt = lq_end_data();

  lq_start_func(lq_linkage_export(), lq_type_word(), "main");
  lq_start_block();
  LqRef r = lq_i_call(add_func, lq_const_word(1), lq_const_word(1));  // TODO: w context
  LqRef printf_func = lq_extern("printf");
  lq_i_call_varargs(printf, r);
  lq_i_emit(LQ_RET, lq_const_word(0));
  LqRef main = lq_end_func();
}
