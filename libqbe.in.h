#pragma once

#include <stdbool.h>
#include <stdint.h>

#define LQ_OPAQUE_STRUCT_DEF(x) \
  typedef struct x {            \
    uint32_t u;                 \
  } x

LQ_OPAQUE_STRUCT_DEF(LqLinkage);
LQ_OPAQUE_STRUCT_DEF(LqType);
LQ_OPAQUE_STRUCT_DEF(LqBlock);
LQ_OPAQUE_STRUCT_DEF(LqRef);

#undef LQ_OPAQUE_STRUCT_DEF

typedef enum LqTarget {
  LQ_TARGET_DEFAULT,        //
  LQ_TARGET_AMD64_APPLE,    //
  LQ_TARGET_AMD64_SYSV,     //
  LQ_TARGET_AMD64_WINDOWS,  //
  LQ_TARGET_ARM64,          //
  LQ_TARGET_ARM64_APPLE,    //
} LqTarget;

void lq_init_target_debug(LqTarget target, const char* debug_flags);
#define lq_init() lq_init_target_debug(LQ_TARGET_DEFAULT, "")
#define lq_init_target(target) lq_init_target_debug(target, "")

LqLinkage lq_linkage_export(void);
LqLinkage lq_linkage_default(void);

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
LqRef lq_func_param_named(LqType type, const char* name);
#define lq_func_param(type) lq_func_param_named(type, NULL)

LqBlock lq_start_block(void);

%%%INSTRUCTIONS%%%

LqRef lq_i_call(LqRef func, int num_args, ...);
LqRef lq_i_call_varargs(LqRef func, int num_args, ...);
LqRef lq_i_ret(LqRef ref);
LqRef lq_i_emit(LqInst instr, LqRef lhs, LqRef rhs);
LqRef lq_i_jump(LqBlock target);
LqRef lq_i_alloc4(LqRef byte_count);
LqRef lq_i_alloc8(LqRef byte_count);
LqRef lq_i_alloc16(LqRef byte_count);

// TODO: phi

LqRef lq_func_end(LqFunc func);


LqType lq_type_start_struct(const char* name, int align /*=0*/);
LqType lq_type_start_union(const char* name, int align);
void lq_type_add_field(LqType type, LqType field);
void lq_type_add_field_with_count(LqType type, LqType field, uint32_t count);
LqType lq_type_end(void);

LqType lq_make_type_opaque(const char* name, int align, int size);
