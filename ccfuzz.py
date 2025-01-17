import random
import sys


class TypeInfo:
    def __init__(self, c_name, size, qbe_name, lq_name, min, max):
        self.c_name = c_name
        self.size = size
        self.qbe_name = qbe_name
        self.lq_name = lq_name
        self.min = min
        self.max = max
        self.fields = None

    def __repr__(self):
        if self.fields:
            return self.c_name + "{" + ",".join(repr(f) for f in self.fields) + "}"
        else:
            return self.c_name


class Field:
    def __init__(self, name, typeinfo):
        self.name = name
        self.typeinfo = typeinfo

    def __repr__(self):
        return f"{self.name}:{self.typeinfo}"


BASIC_TYPES = (
    TypeInfo("int8_t", 1, "b", "LQ_TYPE_SB", -128, 127),
    TypeInfo("int16_t", 2, "h", "LQ_TYPE_SH", -32768, 32767),
    TypeInfo("int32_t", 4, "w", "LQ_TYPE_W", -2147483648, 2147483647),
    TypeInfo("int64_t", 8, "l", "LQ_TYPE_L", -9223372036854775808, 9223372036854775807),
    TypeInfo("uint8_t", 1, "b", "LQ_TYPE_UB", 0, 0xFF),
    TypeInfo("uint16_t", 2, "h", "LQ_TYPE_UH", 0, 0xFFFF),
    TypeInfo("uint32_t", 4, "w", "LQ_TYPE_W", 0, 0xFFFFFFFF),
    TypeInfo("uint64_t", 8, "l", "LQ_TYPE_L", 0, 0xFFFFFFFFFFFFFFFF),
    TypeInfo("float", 4, "s", "LQ_TYPE_S", -1000.0, 1000.0),
    TypeInfo("double", 8, "d", "LQ_TYPE_D", -1000.0, 1000.0),
)

void_typeinfo = TypeInfo("void", 0, None, None, 0, 0)

NON_VOID = "NON_VOID"
VOID_ALLOWED = "VOID_ALLOWED"
struct_counter = 0
qbe_temp_counter = 0


def make_random_struct():
    global struct_counter
    num_fields = random.randrange(1, 16)
    ti = TypeInfo(
        "struct Struct%d" % struct_counter,
        -1,
        ":Struct%d" % struct_counter,
        None,
        -1,
        -1,
    )
    fields = []
    for field_num in range(num_fields):
        field_type = BASIC_TYPES[random.randrange(len(BASIC_TYPES))]
        fields.append(Field("f%d" % field_num, field_type))
    ti.fields = fields
    struct_counter += 1
    return ti


def pick_type(want_void):
    types = []
    if want_void == VOID_ALLOWED:
        types.append(void_typeinfo)
    types.extend(BASIC_TYPES)
    types.append(make_random_struct())
    return types[random.randrange(len(types))]


def genval(ti):
    if isinstance(ti.min, int):
        v = random.randrange(ti.min, ti.max)
        if ti.min == 0:
            return (str(v) + "ULL", str(v))
        else:
            return (str(v) + "LL", str(v))
    else:
        assert isinstance(ti.min, float)
        v = random.uniform(ti.min, ti.max)
        c = "%.3f" % v
        if ti.c_name == "float":
            q = "s_%.3f" % v
        else:
            assert ti.c_name == "double"
            q = "d_%.3f" % v
        return c, q


def gen_qbe_temp():
    global qbe_temp_counter
    ret = "%%tmp%d" % qbe_temp_counter
    qbe_temp_counter += 1
    return ret


def fill_ret_value_c_check_qbe(ti):
    c_code = "return"
    if ti == void_typeinfo:
        c_code += ";"
        pre_q_code = "call "
        post_q_code = ''
    else:
        c_code += " (%s){" % ti.c_name
        qbe_ret_val_name = gen_qbe_temp()
        pre_q_code = "  %s =%s call " % (qbe_ret_val_name, ti.qbe_name)
        if ti in BASIC_TYPES:
            c, q = genval(ti)
        else:
            c = "/*STRUCTTODO*/"
            q = "/*QBETODO*/"
        c_code += c
        cmp_val_name = gen_qbe_temp()
        post_q_code = '  %s =w ceq%s %s, %s\n' % (cmp_val_name, ti.qbe_name, qbe_ret_val_name, q)
        post_q_code += '  jnz %s, @ok, @bad\n' % cmp_val_name
        post_q_code += '@bad\n'
        post_q_code += '  hlt\n'
        post_q_code += '@ok\n'
        c_code += "};"

    return c_code, pre_q_code, post_q_code


def main():
    num_args = random.randrange(0, 16)
    ret_type = pick_type(VOID_ALLOWED)
    c, preq, postq = fill_ret_value_c_check_qbe(ret_type)
    print(c)
    print(preq)
    print(postq)


if __name__ == "__main__":
    main()
