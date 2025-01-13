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
LQ_OPAQUE_STRUCT_DEF(LqFunc);

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

#define lq_type_word ((LqType){LQ_TYPE_W})
#define lq_type_long ((LqType){LQ_TYPE_L})
#define lq_type_single ((LqType){LQ_TYPE_S})
#define lq_type_double ((LqType){LQ_TYPE_D})

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
LqRef lq_data_end(void);

LqRef lq_extern(const char* name);

void lq_func_start(LqLinkage linkage, LqType return_type, const char* name);
LqFunc lq_func_end(void);

// LqRef are function local, so the return value cannot be cached across
// functions.
LqRef lq_ref_for_func(LqFunc func);

#define lq_func_param(type) lq_func_param_named(type, NULL)
LqRef lq_func_param_named(LqType type, const char* name);

LqBlock lq_block_declare_named(const char* name);
#define lq_block_declare() lq_block_declare_named(NULL)

void lq_block_start_previously_declared(LqBlock block);

#define lq_block_start() lq_block_start_named("")
LqBlock lq_block_start_named(const char* name);

%%%INSTRUCTION_DECLARATIONS%%%

void lq_i_ret_void(void);
void lq_i_ret(LqRef val);
void lq_i_jmp(LqBlock block);
void lq_i_jnz(LqRef cond, LqBlock if_true, LqBlock if_false);

LqRef lq_i_calla(LqType result, LqRef func, int num_args, LqType* types, LqRef* args);

LqRef lq_i_call1(LqType result, LqRef func, LqType type0, LqRef arg0);
LqRef lq_i_call2(LqType result, LqRef func, LqType type0, LqRef arg0, LqType type1, LqRef arg1);
LqRef lq_i_call3(LqType result,
                 LqRef func,
                 LqType type0,
                 LqRef arg0,
                 LqType type1,
                 LqRef arg1,
                 LqType type2,
                 LqRef arg2);
LqRef lq_i_call4(LqType result,
                 LqRef func,
                 LqType type0,
                 LqRef arg0,
                 LqType type1,
                 LqRef arg1,
                 LqType type2,
                 LqRef arg2,
                 LqType type3,
                 LqRef arg3);


LqType lq_type_start_struct(const char* name, int align /*=0*/);
LqType lq_type_start_union(const char* name, int align);
void lq_type_add_field(LqType type, LqType field);
void lq_type_add_field_with_count(LqType type, LqType field, uint32_t count);
LqType lq_type_end(void);

LqType lq_make_type_opaque(const char* name, int align, int size);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  /* LIBQBE_H_INCLUDED_ */
