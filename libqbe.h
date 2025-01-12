#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct QbeLinkage { uint32_t u; } QbeLinkage;
typedef struct QbeFunc { uint32_t u; } QbeFunc;
typedef struct QbeData { uint32_t u; } QbeData;
typedef struct QbeType { uint32_t u; } QbeType;
typedef struct QbeBlock { uint32_t u; } QbeBlock;
typedef struct QbePhi { uint32_t u; } QbePhi;
typedef struct QbeInstr { uint32_t u; } QbeInstr;
typedef struct QbeJump { uint32_t u; } QbeJump;
typedef struct QbeRef { uint32_t u; } QbeRef;

QbeType qbe_builtin_type_byte();
QbeType qbe_builtin_type_half();
QbeType qbe_builtin_type_word();
QbeType qbe_builtin_type_long();
QbeType qbe_builtin_type_float();
QbeType qbe_builtin_type_double();

QbeData qbe_begin_data(const char* name, QbeLinkage linkage);
void qbe_data_byte(QbeData data, uint8_t val);
void qbe_data_half(QbeData data, uint16_t val);
void qbe_data_word(QbeData data, uint32_t val);
void qbe_data_long(QbeData data, uint64_t val);
void qbe_data_zero(QbeData data, int count);
// TODO: string/float/double helpers
void qbe_end_data(QbeData data);

QbeFunc qbe_begin_func(const char* name, QbeLinkage linkage, QbeType return_type);

void qbe_func_begin_params(QbeFunc func);
void qbe_func_begin_params_env(QbeFunc func);
void qbe_func_param_add(QbeType type, const char* name);
void qbe_func_end_params(QbeFunc func);
void qbe_func_end_params_variadic(QbeFunc func);

QbeBlock qbe_begin_block(QbeFunc func);
void qbe_block_add_phi(QbePhi phi);
void qbe_block_add_instr(QbeInstr instr);
void qbe_block_add_jump(QbeJump inst);
void qbe_end_block(QbeBlock block);

void qbe_end_func(QbeFunc func);

// TODO: phi

void qbe_instr_store_d(QbeBlock block, QbeRef val, QbeRef into);
void qbe_instr_store_s(QbeBlock block, QbeRef val, QbeRef into);
void qbe_instr_store_l(QbeBlock block, QbeRef val, QbeRef into);
void qbe_instr_store_w(QbeBlock block, QbeRef val, QbeRef into);
void qbe_instr_store_h(QbeBlock block, QbeRef val, QbeRef into);
void qbe_instr_store_b(QbeBlock block, QbeRef val, QbeRef into);

QbeRef qbe_instr_load_d(QbeBlock block, QbeRef from);
QbeRef qbe_instr_load_s(QbeBlock block, QbeRef from);
QbeRef qbe_instr_load_l(QbeBlock block, QbeRef from);
QbeRef qbe_instr_load_sw(QbeBlock block, QbeRef from);
QbeRef qbe_instr_load_uw(QbeBlock block, QbeRef from);
QbeRef qbe_instr_load_sh(QbeBlock block, QbeRef from);
QbeRef qbe_instr_load_uh(QbeBlock block, QbeRef from);
QbeRef qbe_instr_load_sb(QbeBlock block, QbeRef from);
QbeRef qbe_instr_load_ub(QbeBlock block, QbeRef from);

// TODO: blit

QbeRef qbe_instr_alloc_align4(QbeRef byte_count);
QbeRef qbe_instr_alloc_align8(QbeRef byte_count);
QbeRef qbe_instr_alloc_align16(QbeRef byte_count);

typedef enum QbeIntCmp {
  QBE_IC_INVALID,
  QBE_IC_EQ,
  QBE_IC_NE,
  QBE_IC_SLE,
  QBE_IC_SLT,
  QBE_IC_SGE,
  QBE_IC_SGT,
  QBE_IC_ULE,
  QBE_IC_ULT,
  QBE_IC_UGE,
  QBE_IC_UGT,
  QBE_IC_COUNT
} QbeIntCmp;

typedef enum QbeFloatCmp {
  QBE_FC_INVALID,
  QBE_FC_EQ,
  QBE_FC_NE,
  QBE_FC_LE,
  QBE_FC_LT,
  QBE_FC_GE,
  QBE_FC_GT,
  QBE_FC_O,
  QBE_FC_UO,
  QBE_FC_COUNT
} QbeFloatCmp;

QbeRef qbe_instr_compare_int(QbeIntCmp cmp, QbeRef lhs, QbeRef rhs);
QbeRef qbe_instr_compare_float(QbeFloatCmp cmp, QbeRef lhs, QbeRef rhs);


QbeType qbe_begin_type_struct(const char* name, int align /*=0*/);
QbeType qbe_begin_type_union(const char* name, int align);
void qbe_type_add_field(QbeType type, QbeType field);
void qbe_type_add_field_with_count(QbeType type, QbeType field, uint32_t count);
void qbe_end_type(QbeType type);

QbeType qbe_make_type_opaque(const char* name, int align, int size);
