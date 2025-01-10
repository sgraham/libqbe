#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct QQLinkage { uint32_t u; } QQLinkage;
typedef struct QQFunc { uint32_t u; } QQFunc;
typedef struct QQData { uint32_t u; } QQData;
typedef struct QQType { uint32_t u; } QQType;
typedef struct QQBlock { uint32_t u; } QQBlock;
typedef struct QQPhi { uint32_t u; } QQPhi;
typedef struct QQInstr { uint32_t u; } QQInstr;
typedef struct QQJump { uint32_t u; } QQJump;
typedef struct QQRef { uint32_t u; } QQRef;

QQType qq_builtin_type_byte();
QQType qq_builtin_type_half();
QQType qq_builtin_type_word();
QQType qq_builtin_type_long();
QQType qq_builtin_type_float();
QQType qq_builtin_type_double();

QQData qq_begin_data(const char* name, QQLinkage linkage);
void qq_data_byte(QQData data, uint8_t val);
void qq_data_half(QQData data, uint16_t val);
void qq_data_word(QQData data, uint32_t val);
void qq_data_long(QQData data, uint64_t val);
void qq_data_zero(QQData data, int count);
// TODO: string/float/double helpers
void qq_end_data(QQData data);

QQFunc qq_begin_func(const char* name, QQLinkage linkage, QQType return_type);

void qq_func_begin_params(QQFunc func);
void qq_func_begin_params_env(QQFunc func);
void qq_func_param_add(QQType type, const char* name);
void qq_func_end_params(QQFunc func);
void qq_func_end_params_variadic(QQFunc func);

QQBlock qq_begin_block(QQFunc func);
void qq_block_add_phi(QQPhi phi);
void qq_block_add_instr(QQInstr instr);
void qq_block_add_jump(QQJump inst);
void qq_end_block(QQBlock block);

void qq_end_func(QQFunc func);

// TODO: phi

void qq_instr_store_d(QQBlock block, QQRef val, QQRef into);
void qq_instr_store_s(QQBlock block, QQRef val, QQRef into);
void qq_instr_store_l(QQBlock block, QQRef val, QQRef into);
void qq_instr_store_w(QQBlock block, QQRef val, QQRef into);
void qq_instr_store_h(QQBlock block, QQRef val, QQRef into);
void qq_instr_store_b(QQBlock block, QQRef val, QQRef into);

QQRef qq_instr_load_d(QQBlock block, QQRef from);
QQRef qq_instr_load_s(QQBlock block, QQRef from);
QQRef qq_instr_load_l(QQBlock block, QQRef from);
QQRef qq_instr_load_sw(QQBlock block, QQRef from);
QQRef qq_instr_load_uw(QQBlock block, QQRef from);
QQRef qq_instr_load_sh(QQBlock block, QQRef from);
QQRef qq_instr_load_uh(QQBlock block, QQRef from);
QQRef qq_instr_load_sb(QQBlock block, QQRef from);
QQRef qq_instr_load_ub(QQBlock block, QQRef from);

// TODO: blit

QQRef qq_instr_alloc_align4(QQRef byte_count);
QQRef qq_instr_alloc_align8(QQRef byte_count);
QQRef qq_instr_alloc_align16(QQRef byte_count);

typedef enum QQIntCmp {
  QQIC_INVALID,
  QQIC_EQ,
  QQIC_NE,
  QQIC_SLE,
  QQIC_SLT,
  QQIC_SGE,
  QQIC_SGT,
  QQIC_ULE,
  QQIC_ULT,
  QQIC_UGE,
  QQIC_UGT,
  QQIC_COUNT
} QQIntCmp;

typedef enum QQFloatCmp {
  QQFC_INVALID,
  QQIC_EQ,
  QQFC_NE,
  QQFC_LE,
  QQFC_LT
  QQFC_GE,
  QQFC_GT,
  QQFC_O,
  QQFC_UO,
  QQFC_COUNT
} QQFloatCmp;

QQRef qq_instr_compare_int(QQIntCmp cmp, QQRef lhs, QQRef rhs)
QQRef qq_instr_compare_float(QQFloatCmp cmp, QQRef lhs, QQRef rhs)


QQType qq_begin_type_struct(const char* name, int align);
QQType qq_begin_type_union(const char* name, int align);
void qq_type_add_type(QQType type, QQType field);
void qq_end_type(QQType type);

QQType qq_make_type_opaque(const char* name, int align, int size);

