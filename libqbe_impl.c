#include "libqbe.h"

void lq_data_string(const char* str) {
  for (const char* p = str; *p; ++p) {
    lq_data_byte(*p);
  }
}

void lq_data_float(float f) {
  lq_data_word(*(uint32_t*)&f);
}

void lq_data_double(double d) {
  lq_data_long(*(uint64_t*)&d);
}

LqRef lq_func_param(LqType type) {
  return lq_func_param_named(type, NULL);
}

typedef struct LqLinkageData {
  bool export;
  bool tls;
  bool common;
  const char *section_name;
  const char *section_flags;
} LqLinkageData;

LqType lq_begin_type_struct(const char *name, int align) {
  Typ *ty;
  int t, al;
  uint n;

  LqType ret = {ntyp};
  vgrow(&typ, ntyp + 1);
  ty = &typ[ntyp++];
  ty->isdark = 0;
  ty->isunion = 0;
  ty->align = -1;
  ty->size = 0;
  strcpy(ty->name, name);
  if (align != 0) {
    for (al = 0; align /= 2; al++)
      ;
    ty->align = al;
  }
  n = 0;
  ty->fields = vnew(1, sizeof ty->fields[0], PHeap);
  return ret;
}

void lq_type_add_field_with_count(LqType type, LqType field, uint32_t count) {
#if 0
  Typ *ty1;
  int n, a, al, type;
  uint64_t sz, s;

  Typ *ty = &typ[type.u];
  n = 0;
  sz = 0;
  al = ty->align;

  ty1 = &typ[field.u];
  s = ty1->size;
  a = ty1->align;

  if (a > al)
    al = a;
  a = (1 << a) - 1;
  a = ((sz + a) & ~a) - sz;
  if (a) {
    if (n < NField) {
      fld[n].type = FPad;
      fld[n].len = a;
      n++;
    }
  }
  sz += a + count * s;
  if (field.u > LQ_LAST_BASIC_TYPE) {
    s = field.u;
  }
  for (; c >= 0 && n < NField; c--, n++) {
    fld[n].type = type;
    fld[n].len = s;
  }
#endif
}
void lq_type_add_field(LqType type, LqType field) {
  lq_type_add_field_with_count(type, field, 1);
}

void lq_end_type(LqType type) {
#if 0
  fld[n].type = FEnd;
  a = 1 << al;
  if (sz < ty->size) {
    sz = ty->size;
  }
  ty->size = (sz + a - 1) & ~a;
  ty->align = al;
#endif
}

LqType lq_make_type_opaque(const char *name, int align, int size) {
  abort();
}
