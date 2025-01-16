#include "libqbe.h"

#include <string.h>

void compile_if_then_else_using_memory(void) {
  lq_func_start(lq_linkage_default, lq_type_word, "conditional_using_memory");
  LqRef a = lq_func_param(lq_type_word);
  LqRef b = lq_func_param(lq_type_word);

  // Forward references to blocks for the then, else, and after the conditional.
  LqBlock then = lq_block_declare();
  LqBlock els = lq_block_declare();
  LqBlock after = lq_block_declare();

  // There's not a way to write to a variable multiple times (i.e. emit non-SSA
  // form), because all lq_i_* calls return a new temporary with the result.
  // Instead, store to an alloca, and load the result at the end of the
  // conditional.
  LqRef result = lq_i_alloc4(lq_const_int(4));

  // Implement the silly expression: `a<b ? a+b : a-b`.
  LqRef cmp = lq_i_csltw(lq_type_word, a, b);
  lq_i_jnz(cmp, then, els);

  lq_block_start(then);
  LqRef tc = lq_i_add(lq_type_word, a, b);  // Do add in the 'then'.
  lq_i_storew(tc, result);
  lq_i_jmp(after);

  lq_block_start(els);
  LqRef ec = lq_i_sub(lq_type_word, a, b);  // And sub in the 'else'.
  lq_i_storew(ec, result);
  lq_i_jmp(after);

  lq_block_start(after);
  lq_i_ret(lq_i_loadsw(lq_type_word, result));

  lq_func_end();
}

void compile_if_then_else_using_phi(void) {
  lq_func_start(lq_linkage_default, lq_type_word, "conditional_using_phi");
  LqRef a = lq_func_param(lq_type_word);
  LqRef b = lq_func_param(lq_type_word);

  // Forward references to blocks for the then, else, and after the conditional.
  LqBlock then = lq_block_declare();
  LqBlock els = lq_block_declare();
  LqBlock after = lq_block_declare();

  // In this version, we implement the same conditional, but using a phi
  // instruction to merge the results.

  // Implement the silly expression: `a<b ? a+b : a-b`.
  LqRef cmp = lq_i_csltw(lq_type_word, a, b);
  lq_i_jnz(cmp, then, els);

  lq_block_start(then);
  LqRef tc = lq_i_add(lq_type_word, a, b);  // Do add in the 'then'.
  lq_i_jmp(after);

  lq_block_start(els);
  LqRef ec = lq_i_sub(lq_type_word, a, b);  // And sub in the 'else'.
  lq_i_jmp(after);

  lq_block_start(after);
  // Use `tc` if flow went through the `then` branch, and `ec` if it went
  // through the `else` branch.
  LqRef result = lq_i_phi(lq_type_word, then, tc, els, ec);
  lq_i_ret(result);

  lq_func_end();
}

void compile_passing_aggregates_by_value(void) {
  // Can view type layout with 'T' in lq_init() debug_flags.

  // struct SomeStruct { uint64_t a; uint64_t b; float c; };
  lq_type_struct_start("SomeStruct", 0);
  lq_type_add_field(lq_type_long);
  lq_type_add_field(lq_type_long);
  lq_type_add_field(lq_type_single);
  LqType some_struct = lq_type_struct_end();
  printf("somestruct.u = %u\n", some_struct.u);

  lq_type_struct_start("Paddy", 0);
  lq_type_add_field(lq_type_byte);
  lq_type_add_field(lq_type_long);
  lq_type_add_field(lq_type_word);
  lq_type_add_field(lq_type_double);
  lq_type_struct_end();

  lq_type_struct_start("LateAlign", 0);
  lq_type_add_field_with_count(lq_type_byte, 99);
  lq_type_add_field(lq_type_long);
  lq_type_struct_end();

  lq_func_start(lq_linkage_default, lq_type_long, "passing_aggregate_by_value");
  LqRef p0 = lq_func_param(lq_type_word);
  LqRef agg0 = lq_func_param(some_struct);

  LqRef second_field_ptr = lq_i_add(lq_type_long, agg0, lq_const_int(8));
  LqRef second_field = lq_i_load(lq_type_long, second_field_ptr);
  LqRef p0e = lq_i_extuw(p0);
  LqRef res = lq_i_add(lq_type_long, p0e, second_field);
  lq_i_ret(res);

  lq_func_end();
}

void compile_data_references(void) {
  const char* strdata = "this is some text";
  lq_data_start(lq_linkage_default, "some_string_bytes");
  lq_data_string(strdata);
  LqSymbol string_bytes = lq_data_end();

  lq_data_start(lq_linkage_default, "some_string_obj");
  lq_data_ref(string_bytes, 0);
  lq_data_long(strlen(strdata));
  lq_data_end();
}

int main(void) {
  lq_init(LQ_TARGET_AMD64_SYSV, stdout, "");

  compile_if_then_else_using_memory();

  compile_if_then_else_using_phi();

  compile_passing_aggregates_by_value();

  compile_data_references();

  lq_shutdown();
}
