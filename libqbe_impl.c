static PState _ps;

// Blk lifetimes are per-func
// This array is ~2M, perhaps lq_init arg to size this?
static Blk _block_arena[8<<10];
static int _num_blocks;

// Lnk lifetimes are lq_init() scoped
static Lnk _linkage_arena[1<<10];
static int _num_linkages;

typedef enum LqInitStatus {
  LQIS_UNINITIALIZED = 0,
  LQIS_INITIALIZED_EMIT_FIN = 1,
  LQIS_INITIALIZED_NO_FIN = 2,
} LqInitStatus;
static LqInitStatus lq_initialized;

static uint _ntyp;

static int _dbg_name_counter;

static void _gen_dbg_name(char* into, const  char* prefix) {
  sprintf(into, "%s_%d", prefix ? prefix : "", _dbg_name_counter++);
}

#define LQ_NAMED_IF_DEBUG(into, provided) \
  if (dbg) {                              \
    _gen_dbg_name(into, provided);        \
  }

#define LQ_COUNTOF(a) (sizeof(a)/sizeof(a[0]))
#define LQ_COUNTOFI(a) ((int)(sizeof(a)/sizeof(a[0])))

#ifdef _MSC_VER
#define alloca _alloca
#endif

static void err(char* s, ...) {
  va_list args;
  va_start(args, s);
  fprintf(stderr, "libqbe: ");
  vfprintf(stderr, s, args);
  fprintf(stderr, "\n");
  va_end(args);
  // TODO: setjmp/longjmp w/ clean up
  exit(1);
}

LqLinkage lq_linkage_create(int alignment,
                            bool exported,
                            bool tls,
                            bool common,
                            const char* section_name,
                            const char* section_flags) {
  LQ_ASSERT(_num_linkages < LQ_COUNTOFI(_linkage_arena));
  LqLinkage ret = {_num_linkages++};
  _linkage_arena[ret.u] = (Lnk){
      .export = exported,
      .thread = tls,
      .common = common,
      .align = alignment,
      .sec = (char*)section_name,
      .secf = (char*)section_flags,
  };
  return ret;
}

static Lnk* _linkage_to_internal_lnk(LqLinkage linkage) {
  return &_linkage_arena[linkage.u];
}

MAKESURE(ref_sizes_match, sizeof(LqRef) == sizeof(Ref));
MAKESURE(ref_has_expected_size, sizeof(uint32_t) == sizeof(Ref));

static Ref _lqref_to_internal_ref(LqRef ref) {
  Ref ret;
  memcpy(&ret, &ref, sizeof(Ref));
  return ret;
}

static LqRef _internal_ref_to_lqref(Ref ref) {
  LqRef ret;
  memcpy(&ret, &ref, sizeof(Ref));
  return ret;
}

static int _lqtype_to_cls_and_ty(LqType type, int* ty) {
  LQ_ASSERT(type.u != LQ_TYPE_C);
  if (type.u > LQ_TYPE_0) {
    *ty = type.u;
    return LQ_TYPE_C;
  } else {
    *ty = Kx;
    return type.u;
  }
}

void lq_init(LqTarget target, FILE* output, const char* debug_names) {
  LQ_ASSERT(lq_initialized == LQIS_UNINITIALIZED);

  _dbg_name_counter = 0;

  _num_linkages = 0;
  // These have to match the header for lq_linkage_default/export.
  LqLinkage def = lq_linkage_create(8, false, false, false, NULL, NULL);
  LQ_ASSERT(def.u == 0); (void)def;
  LqLinkage exp = lq_linkage_create(8, true, false, false, NULL, NULL);
  LQ_ASSERT(exp.u == 1); (void)exp;

  (void)qbe_parse_tmpref; // TODO
  (void)qbe_main_dbgfile;
  (void)qbe_main_data;

  switch (target) {
    case LQ_TARGET_AMD64_APPLE:
      T = T_amd64_apple;
      break;
    case LQ_TARGET_AMD64_SYSV:
      T = T_amd64_sysv;
      break;
    case LQ_TARGET_AMD64_WINDOWS:
      T = T_amd64_win;
      break;
    case LQ_TARGET_ARM64:
      T = T_arm64;
      break;
    case LQ_TARGET_ARM64_APPLE:
      T = T_arm64_apple;
      break;
    case LQ_TARGET_RV64:
      T = T_rv64;
      break;
    default: {
#if defined(__APPLE__) && defined(__MACH__)
#  if defined(__aarch64__)
      T = T_arm64_apple;
#  else
      T = T_amd64_apple;
#  endif
#elif defined(_WIN32)
#  if defined(__aarch64__)
#    error port win arm64
#  else
      T = T_amd64_win;
#  endif
#else
#  if defined(__aarch64__)
      T = T_arm64;
#  elif defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
      T = T_amd64_sysv;
#  elif defined(__riscv)
      T = T_rv64;
#  else
#    error port unknown
#  endif
#endif
    }
  }

  outf = output;

  memset(debug, 0, sizeof(debug));
  for (const char* d = debug_names; *d; ++d) {
    debug[(int)*d] = 1;
  }

  _ntyp = 0;
  typ = vnew(0, sizeof(typ[0]), PHeap);

  dbg = debug_names[0] != 0;
  lq_initialized = dbg ? LQIS_INITIALIZED_NO_FIN : LQIS_INITIALIZED_EMIT_FIN;
}

void lq_shutdown(void) {
  LQ_ASSERT(lq_initialized != LQIS_UNINITIALIZED);
  if (lq_initialized == LQIS_INITIALIZED_EMIT_FIN) {
    T.emitfin(outf);
  }
  // TODO: pool flushes, etc
  lq_initialized = LQIS_UNINITIALIZED;
}

void lq_func_start(LqLinkage linkage, LqType return_type, const char* name) {
  Lnk* lnk = _linkage_to_internal_lnk(linkage);
  lnk->align = 16;

  curb = 0;
  _num_blocks = 0;
  curi = insb;
  curf = alloc(sizeof *curf);
  curf->ntmp = 0;
  curf->ncon = 2;
  curf->tmp = vnew(curf->ntmp, sizeof curf->tmp[0], PFn);
  curf->con = vnew(curf->ncon, sizeof curf->con[0], PFn);
  for (int i = 0; i < Tmp0; ++i) {
    if (T.fpr0 <= i && i < T.fpr0 + T.nfpr) {
      newtmp(0, Kd, curf);
    } else {
      newtmp(0, Kl, curf);
    }
  }
  curf->con[0].type = CBits;
  curf->con[0].bits.i = 0xdeaddead; /* UNDEF */
  curf->con[1].type = CBits;
  curf->lnk = *lnk;
  curf->leaf = 1;
  blink = &curf->start;
  rcls = _lqtype_to_cls_and_ty(return_type, &curf->retty);
  strncpy(curf->name, name, NString - 1);
  _ps = PLbl;

  lq_block_start();
}

LqRef lq_func_param_named(LqType type, const char* name) {
  int ty;
  int k = _lqtype_to_cls_and_ty(type, &ty);
  Ref r = newtmp(0, k, curf);
  LQ_NAMED_IF_DEBUG(curf->tmp[r.val].name, name);
  // TODO: env ptr, varargs
  if (k == Kc) {
    *curi = (Ins){Oparc, Kl, r, {TYPE(ty)}};
  } else if (k >= Ksb) {
    *curi = (Ins){Oparsb + (k - Ksb), Kw, r, {R}};
  } else {
    *curi = (Ins){Opar, k, r, {R}};
  }
  ++curi;
  return _internal_ref_to_lqref(r);
}

#if 0
	Con c;

	memset(&c, 0, sizeof c);
	switch (next()) {
	default:
		return R;
	case Ttmp:
		return tmpref(tokval.str);
	case Tthread:
		c.sym.type = SThr;
		expect(Tglo);
		/* fall through */
	case Tglo:
		c.type = CAddr;
		c.sym.id = intern(tokval.str);
		break;
	}
	return newcon(&c, curf);
#endif

LqRef lq_const_int(int64_t i) {
  Con c = {0};
  c.type = CBits;
  c.bits.i = i;
  return _internal_ref_to_lqref(newcon(&c, curf));
}

LqRef lq_const_single(float f) {
  Con c = {0};
  c.type = CBits;
  c.bits.s = f;
  c.flt = 1;
  return _internal_ref_to_lqref(newcon(&c, curf));
}

LqRef lq_const_double(double d) {
  Con c = {0};
  c.type = CBits;
  c.bits.d = d;
  c.flt = 2;
  return _internal_ref_to_lqref(newcon(&c, curf));
}

// This has to return a LqFunc that's the name that we re-lookup on use rather
// than directly a Ref because Con Refs are stored per function (so when calling
// the function, we need to create a new one in the caller).
LqFunc lq_func_end(void) {
  if (!curb) {
    err("empty function");
  }
  if (curb->jmp.type == Jxxx) {
    err("last block misses jump");
  }
  curf->mem = vnew(0, sizeof curf->mem[0], PFn);
  curf->nmem = 0;
  curf->nblk = _num_blocks;
  curf->rpo = 0;
  for (Blk* b = curf->start; b; b = b->link) {
    LQ_ASSERT(b->dlink == 0);
  }
  memset(tmph, 0, tmphcap * sizeof tmph[0]);  // ??
  qbe_parse_typecheck(curf);

  LqFunc ret = {intern(curf->name)};

  qbe_main_func(curf);

  return ret;
}

LqRef lq_ref_for_func(LqFunc func) {
  Con c = {0};
  c.type = CAddr;
  c.sym.id = func.u;
  Ref ret = newcon(&c, curf);
  return _internal_ref_to_lqref(ret);
}

LqRef lq_extern(const char* name) {
  Con c = {0};
  c.type = CAddr;
  c.sym.id = intern((char*)name);
  Ref ret = newcon(&c, curf);
  return _internal_ref_to_lqref(ret);
}

LqBlock lq_block_declare_named(const char* name) {
  LQ_ASSERT(_num_blocks < LQ_COUNTOFI(_block_arena));
  LqBlock ret = {_num_blocks++};
  Blk* blk = &_block_arena[ret.u];
  memset(blk, 0, sizeof(Blk));
  blk->id = ret.u;
  LQ_NAMED_IF_DEBUG(blk->name, name);
  return ret;
}

void lq_block_start_previously_declared(LqBlock block) {
  Blk* b = &_block_arena[block.u];
  if (curb && curb->jmp.type == Jxxx) {
    qbe_parse_closeblk();
    curb->jmp.type = Jjmp;
    curb->s1 = b;
  }
  if (b->jmp.type != Jxxx) {
    err("multiple definitions of block @%s", b->name);
  }
  *blink = b;
  curb = b;
  plink = &curb->phi;
  _ps = PPhi;
}

LqBlock lq_block_start_named(const char* name) {
  LqBlock new = lq_block_declare_named(name);
  lq_block_start_previously_declared(new);
  return new;
}

void lq_i_ret(LqRef val) {
  LQ_ASSERT(_ps == PIns || _ps == PPhi);
  curb->jmp.type = Jretw + rcls;
  if (val.u == 0) {
    curb->jmp.type = Jret0;
  } else if (rcls != K0) {
    Ref r = _lqref_to_internal_ref(val);
    if (req(r, R)) {
      err("invalid return value");
    }
    curb->jmp.arg = r;
  }
  qbe_parse_closeblk();
  _ps = PLbl;
}

void lq_i_ret_void(void) {
  lq_i_ret((LqRef){0});  // TODO: not sure if this is correct == {RTmp, 0}.
}

LqRef lq_i_calla(LqType result,
                 LqRef func,
                 bool is_varargs,
                 int num_args,
                 LqType* types,
                 LqRef* args) {
  if (is_varargs) {
    LQ_ASSERT(curi - insb < NIns);
    *curi++ = (Ins){.op = Oargv};
  }

  LQ_ASSERT(_ps == PIns || _ps == PPhi);
  // args are inserted into instrs first, then the call
  for (int i = 0; i < num_args; ++i) {
    LQ_ASSERT(curi - insb < NIns);
    int ty;
    int k = _lqtype_to_cls_and_ty(types[i], &ty);
    Ref r = _lqref_to_internal_ref(args[i]);
    // TODO: env
    if (k == Kc) {
      *curi = (Ins){Oargc, Kl, R, {TYPE(ty), r}};
    } else if (k >= Ksb) {
      *curi = (Ins){Oargsb + (k - Ksb), Kw, R, {r}};
    } else {
      *curi = (Ins){Oarg, k, R, {r}};
    }
    ++curi;
  }

  Ref tmp;

  {
    LQ_ASSERT(curi - insb < NIns);
    int ty;
    int k = _lqtype_to_cls_and_ty(result, &ty);
    curf->leaf = 0;
    *curi = (Ins){0};
    curi->op = Ocall;
    curi->arg[0] = _lqref_to_internal_ref(func);
    if (k == Kc) {
      k = Kl;
      curi->arg[1] = TYPE(ty);
    }
    if (k >= Ksb) {
      k = Kw;
    }
    curi->cls = k;
    tmp = newtmp(NULL, k, curf);
    curi->to = tmp;
    ++curi;
  }
  _ps = PIns;
  return _internal_ref_to_lqref(tmp);
}

LqRef _lq_i_call_implv(bool is_varargs, int num_args, LqType result, LqRef func, ...) {
  va_list ap;
  va_start(ap, func);

  LQ_ASSERT(num_args < 16);
  LqType* types = alloca(sizeof(LqType) * num_args);
  LqRef* args = alloca(sizeof(LqRef) * num_args);
  for (int i = 0; i < num_args; ++i) {
    types[i] = va_arg(ap, LqType);
    args[i] = va_arg(ap, LqRef);
  }
  va_end(ap);
  return lq_i_calla(result, func, is_varargs, num_args, types, args);
}

LqRef _normal_two_op_instr(int op, LqType size_class, LqRef arg0, LqRef arg1) {
  LQ_ASSERT(size_class.u >= LQ_TYPE_W && size_class.u <= LQ_TYPE_D);
  LQ_ASSERT(_ps == PIns || _ps == PPhi);
  LQ_ASSERT(curi - insb < NIns);
  Ref tmp = newtmp(NULL, Kx, curf);
  LQ_NAMED_IF_DEBUG(curf->tmp[tmp.val].name, NULL);
  curi->op = op;
  curi->cls = size_class.u;
  curi->to = tmp;
  curi->arg[0] = _lqref_to_internal_ref(arg0);
  curi->arg[1] = _lqref_to_internal_ref(arg1);
  ++curi;
  _ps = PIns;
  return _internal_ref_to_lqref(tmp);
}

LqRef lq_i_add(LqType size_class, LqRef lhs, LqRef rhs) {
  return _normal_two_op_instr(Oadd, size_class, lhs, rhs);
}

#if 0
void lq_data_start(LqLinkage linkage, const char* name) {
}
#endif
#if 0
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
#endif

#if 0
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
#  if 0
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
#  endif
}
void lq_type_add_field(LqType type, LqType field) {
  lq_type_add_field_with_count(type, field, 1);
}

void lq_end_type(LqType type) {
#  if 0
  fld[n].type = FEnd;
  a = 1 << al;
  if (sz < ty->size) {
    sz = ty->size;
  }
  ty->size = (sz + a - 1) & ~a;
  ty->align = al;
#  endif
}

LqType lq_make_type_opaque(const char *name, int align, int size) {
  abort();
}
#endif
