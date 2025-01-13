static PState _ps;
static Blk _block_arena[1024];  // TODO: lq_init arg (this is per func)
static int _num_blocks;

typedef enum LqInitStatus {
  LQIS_UNINITIALIZED = 0,
  LQIS_INITIALIZED_EMIT_FIN = 1,
  LQIS_INITIALIZED_NO_FIN = 2,
} LqInitStatus;
static LqInitStatus lq_initialized;

static uint lq_ntyp;

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

void lq_init(LqTarget target, FILE* output, const char* debug_names) {
  assert(lq_initialized == LQIS_UNINITIALIZED);

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
      abort();
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
      // TODO
      // T = T_amd64_windows;
      T = T_amd64_sysv;
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

  for (const char* d = debug_names; *d; ++d) {
    debug[(int)*d] = 1;
  }

  lq_ntyp = 0;
  typ = vnew(0, sizeof(typ[0]), PHeap);

  dbg = debug_names[0] != 0;
  lq_initialized = dbg ? LQIS_INITIALIZED_NO_FIN : LQIS_INITIALIZED_EMIT_FIN;
}

void lq_shutdown(void) {
  assert(lq_initialized != LQIS_UNINITIALIZED);
  if (lq_initialized == LQIS_INITIALIZED_EMIT_FIN) {
    T.emitfin(outf);
  }
  // TODO: pool flushes, etc
  lq_initialized = LQIS_UNINITIALIZED;
}

static Lnk _linkage_to_internal_lnk(LqLinkage linkage) {
  (void)linkage;
  return (Lnk){0};  // TODO
}

void lq_func_start(LqLinkage linkage, LqType return_type, const char* name) {
  Lnk lnk = _linkage_to_internal_lnk(linkage);
  lnk.align = 16;

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
  curf->lnk = lnk;
  curf->leaf = 1;
  blink = &curf->start;
  curf->retty = Kx;
  rcls = return_type.u;
  assert(return_type.u != LQ_TYPE_C);
  if (return_type.u > LQ_TYPE_0) {
    curf->retty = return_type.u;
    rcls = LQ_TYPE_C;
  }
  strncpy(curf->name, name, NString - 1);
  _ps = PLbl;

  lq_block_start();
}

LqLinkage lq_linkage_default(void) {
  return (LqLinkage){0};  // TODO
}

LqLinkage lq_linkage_export(void) {
  return (LqLinkage){0};  // TODO
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

void lq_i_ret(LqRef val) {
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

LqRef lq_func_end(void) {
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
    assert(b->dlink == 0);
  }
  memset(tmph, 0, tmphcap * sizeof tmph[0]);  // ??
  qbe_parse_typecheck(curf);

  qbe_main_func(curf);
  return (LqRef){0};
}

LqBlock lq_block_declare_named(const char* name) {
  LqBlock ret = {_num_blocks++};
  Blk* blk = &_block_arena[ret.u];
  memset(blk, 0, sizeof(Blk));
  blk->id = ret.u;
  strcpy(blk->name, name);
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
