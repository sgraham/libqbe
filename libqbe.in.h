#ifndef LIBQBE_H_INCLUDED_
#define LIBQBE_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define LQ_OPAQUE_STRUCT_DEF(x) \
  typedef struct x {            \
    uint32_t u;                 \
  } x

LQ_OPAQUE_STRUCT_DEF(LqLinkage);
LQ_OPAQUE_STRUCT_DEF(LqType);
LQ_OPAQUE_STRUCT_DEF(LqBlock);
LQ_OPAQUE_STRUCT_DEF(LqRef);
LQ_OPAQUE_STRUCT_DEF(LqSymbol);

#undef LQ_OPAQUE_STRUCT_DEF

typedef enum LqTarget {
  LQ_TARGET_DEFAULT,        //
  LQ_TARGET_AMD64_APPLE,    //
  LQ_TARGET_AMD64_SYSV,     //
  LQ_TARGET_AMD64_WINDOWS,  //
  LQ_TARGET_ARM64,          //
  LQ_TARGET_ARM64_APPLE,    //
  LQ_TARGET_RV64,           //
} LqTarget;

/*
debug_flags string can contain the following characters to cause QBE to output
information to stderr while compiling. NOTE: no final assembly will be emitted
if string other than "" is specified.
- P: parsing
- M: memory optimization
- N: ssa construction
- C: copy elimination
- F: constant folding
- A: abi lowering
- I: instruction selection
- L: liveness
- S: spilling
- R: reg. allocation
- T: types
*/
void lq_init(LqTarget target /*=LQ_TARGET_DEFAULT*/,
             FILE* output /*=stdout*/,
             const char* debug_flags /*=""*/);
void lq_shutdown(void);

LqLinkage lq_linkage_create(int alignment,
                            bool exported,
                            bool tls,
                            bool common,
                            const char* section_name,
                            const char* section_flags);
#define lq_linkage_default ((LqLinkage){0})
#define lq_linkage_export ((LqLinkage){1})

// These values must match internal Kw, Kl, etc.
typedef enum LqTypeKind {
  LQ_TYPE_W = 0,
  LQ_TYPE_L = 1,
  LQ_TYPE_S = 2,
  LQ_TYPE_D = 3,
  LQ_TYPE_SB = 4,
  LQ_TYPE_UB = 5,
  LQ_TYPE_SH = 6,
  LQ_TYPE_UH = 7,
  LQ_TYPE_C = 8,          // User-defined class
  LQ_TYPE_0 = 9,          // void
  LQ_TYPE_E = -2,         // error
  LQ_TYPE_M = LQ_TYPE_L,  // memory
} LqTypeKind;

#define lq_type_void ((LqType){LQ_TYPE_0})
#define lq_type_word ((LqType){LQ_TYPE_W})
#define lq_type_long ((LqType){LQ_TYPE_L})
#define lq_type_single ((LqType){LQ_TYPE_S})
#define lq_type_double ((LqType){LQ_TYPE_D})
#define lq_type_byte ((LqType){LQ_TYPE_UB})
#define lq_type_half ((LqType){LQ_TYPE_UH})
#define lq_type_sbyte ((LqType){LQ_TYPE_SB})
#define lq_type_shalf ((LqType){LQ_TYPE_SH})
#define lq_type_ubyte ((LqType){LQ_TYPE_UB})
#define lq_type_uhalf ((LqType){LQ_TYPE_UH})

void lq_type_struct_start(const char* name, int align /*=0 for natural*/);
void lq_type_add_field(LqType field);
void lq_type_add_field_with_count(LqType field, uint32_t count);
LqType lq_type_struct_end(void);

LqRef lq_const_int(int64_t i);
LqRef lq_const_single(float f);
LqRef lq_const_double(double d);

void lq_data_start(LqLinkage linkage, const char* name);
void lq_data_byte(uint8_t val);
void lq_data_half(uint16_t val);
void lq_data_word(uint32_t val);
void lq_data_long(uint64_t val);
void lq_data_string(const char* str);
void lq_data_single(float f);
void lq_data_double(double d);
LqSymbol lq_data_end(void);

void lq_func_start(LqLinkage linkage, LqType return_type, const char* name);
LqSymbol lq_func_end(void);

void lq_func_dump_current(FILE* to);

// LqRef are function local, so the return value from cannot be cached across
// functions.
LqRef lq_ref_for_symbol(LqSymbol sym);

LqRef lq_extern(const char* name);

#define lq_func_param(type) lq_func_param_named(type, NULL)
LqRef lq_func_param_named(LqType type, const char* name);

#define lq_block_declare() lq_block_declare_named(NULL)
LqBlock lq_block_declare_named(const char* name);

void lq_block_start(LqBlock block);

#define lq_block_declare_and_start() lq_block_declare_and_start_named(NULL)
LqBlock lq_block_declare_and_start_named(const char* name);

void lq_i_ret_void(void);
void lq_i_ret(LqRef val);
void lq_i_jmp(LqBlock block);
void lq_i_jnz(LqRef cond, LqBlock if_true, LqBlock if_false);

// TODO: only 2-branch phi supported currently
LqRef lq_i_phi(LqType size_class, LqBlock block0, LqRef val0, LqBlock block1, LqRef val1);

LqRef lq_i_calla(LqType result,
                 LqRef func,
                 bool is_varargs,
                 int num_args,
                 LqType* types,
                 LqRef* args);

LqRef lq_i_call_implv(bool is_varargs, int num_args, LqType result, LqRef func, ...);

#define LQ_EXPAND(x) x
#define LQ___NARGS(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, VAL, ...) VAL
#define LQ_NARGS_1(...)                                                                \
  LQ_EXPAND(LQ___NARGS(__VA_ARGS__, 6, ERROR_UNPAIRED_TYPE_AND_REF, 5,                 \
                       ERROR_UNPAIRED_TYPE_AND_REF, 4, ERROR_UNPAIRED_TYPE_AND_REF, 3, \
                       ERROR_UNPAIRED_TYPE_AND_REF, 2, ERROR_UNPAIRED_TYPE_AND_REF, 1, \
                       ERROR_UNPAIRED_TYPE_AND_REF, 0, ERROR_UNPAIRED_TYPE_AND_REF))
#define LQ_AUGMENTER(...) unused, __VA_ARGS__
#define LQ_NARGS(...) LQ_NARGS_1(LQ_AUGMENTER(__VA_ARGS__))

// This is just a helper, so you can write:
//
//   lq_i_call(result, func, type0, arg0)
//
// or
//
//   lq_i_call(result, func, type0, arg0, type1, arg1, type2, arg2)
//
// etc. It will compile to a non-existent function if you list a mismatched pair
// of type/args.
//
// Normally, you'll probably want to use lq_i_acall() to pass explicitly built
// arrays from the front end anyway, but sometimes directly passing the
// arguments is convenient.
#define lq_i_call(result_type, ...) \
  lq_i_call_implv(/*va=*/false, LQ_NARGS(__VA_ARGS__), result_type, __VA_ARGS__)

#define lq_i_call_varargs(result_type, ...) \
  lq_i_call_implv(/*va=*/true, LQ_NARGS(__VA_ARGS__), result_type, __VA_ARGS__)

%%%INSTRUCTION_DECLARATIONS%%%

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  /* LIBQBE_H_INCLUDED_ */
