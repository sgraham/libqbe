#include "libqbe.h"

typedef struct QbeLinkageData {
  bool export;
  bool tls;
  bool common;
  const char* section_name;
  const char* section_flags;
} QbeLinkageData;

QbeType qbe_begin_type_struct(const char* name, int align) {
	Typ *ty;
	int t, al;
	uint n;

  QbeType ret = {ntyp};
  vgrow(&typ, ntyp+1);
	ty = &typ[ntyp++];
	ty->isdark = 0;
	ty->isunion = 0;
	ty->align = -1;
	ty->size = 0;
  strcpy(ty->name, name);
  if (align != 0) {
    for (al=0; align /= 2; al++)
      ;
    ty->align = al;
  }
  n = 0;
  ty->fields = vnew(1, sizeof ty->fields[0], PHeap);
  return ret;
}

void qbe_type_add_field_with_count(QbeType type, QbeType field, uint32_t count) {
  Typ* ty1;
  int n, a, al, type;
  uint64_t sz, s;

  Typ* ty = &typ[type.u];
  n = 0;
  sz = 0;
  al = ty->align;
  
  ty1 = &typ[field.u];
  s = ty1->size;
  a = ty1->align;
  
  if (a > al)
    al = a;
  a =  (1 << a) - 1;
  a = ((sz + a) & ~a) - sz;
  if (a) {
    if (n < NField) {
      fld[n].type = FPad;
      fld[n].len = a;
      n++;
    }
  }
  sz += a + count*s;
  if (field.u > QBE_LAST_BASIC_TYPE) {
    s = field.u;
  }
  for (; c>= 0 && n<NField; c--, n++) {
    fld[n].type = type;
    fld[n].len = s;
  }
}
void qbe_type_add_field(QbeType type, QbeType field) {
  qbe_type_add_field_with_count(type, field, 1);
}

void qbe_end_type(QbeType type) {
  fld[n].type = FEnd;
  a = 1 << al;
  if (sz < ty->size) {
    sz = ty->size;
  }
  ty->size = (sz + a - 1) & ~a;
  ty->align = al;
}

QbeType qbe_make_type_opaque(const char* name, int align, int size) {
}
