#!/usr/bin/env python

import random
import subprocess
import sys


class TypeInfo:
    def __init__(self, c_name, size, qbe_name, qbe_size_class, lq_name, min, max):
        self.c_name = c_name
        self.size = size
        self.qbe_name = qbe_name
        self.qbe_size_class = qbe_size_class
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
    TypeInfo("int8_t", 1, "sb", "w", "LQ_TYPE_SB", -128, 127),
    TypeInfo("int16_t", 2, "sh", "w", "LQ_TYPE_SH", -32768, 32767),
    TypeInfo("int32_t", 4, "w", "w", "LQ_TYPE_W", -2147483648, 2147483647),
    TypeInfo(
        "int64_t", 8, "l", "l", "LQ_TYPE_L", -9223372036854775808, 9223372036854775807
    ),
    TypeInfo("uint8_t", 1, "ub", "w", "LQ_TYPE_UB", 0, 0xFF),
    TypeInfo("uint16_t", 2, "uh", "w", "LQ_TYPE_UH", 0, 0xFFFF),
    TypeInfo("uint32_t", 4, "w", "w", "LQ_TYPE_W", 0, 0xFFFFFFFF),
    TypeInfo("uint64_t", 8, "l", "l", "LQ_TYPE_L", 0, 0xFFFFFFFFFFFFFFFF),
    TypeInfo("float", 4, "s", "s", "LQ_TYPE_S", -1000.0, 1000.0),
    TypeInfo("double", 8, "d", "d", "LQ_TYPE_D", -1000.0, 1000.0),
)

void_typeinfo = TypeInfo("void", 0, None, None, None, 0, 0)

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
    # types.append(make_random_struct())
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
        pre_q_code = "call $func_to_check("
        post_q_code = ""
    else:
        c_code += " (%s){" % ti.c_name
        qbe_ret_val_name = gen_qbe_temp()
        pre_q_code = "  %s =%s call $func_to_check(" % (
            qbe_ret_val_name,
            ti.qbe_size_class,
        )
        if ti in BASIC_TYPES:
            c, q = genval(ti)
        else:
            c = "/*STRUCTTODO*/"
            q = "/*QBETODO*/"
        c_code += c
        c_code += "};"
        cmp_val_name = gen_qbe_temp()
        post_q_code = "  %s =w ceq%s %s, %s\n" % (
            cmp_val_name,
            ti.qbe_size_class,
            qbe_ret_val_name,
            q,
        )
        post_q_code += "  jnz %s, @ok, @bad\n" % cmp_val_name
        post_q_code += "@bad\n"
        post_q_code += "  hlt\n"
        post_q_code += "@ok\n"

    return c_code, pre_q_code, post_q_code


def call_from_qbe_check_in_c(i, ti):
    c, q = genval(ti)
    q_arg = "%s %s" % (ti.qbe_name, q)
    if ti.c_name == "float":
        c_check = "float_check(a%d, %s);" % (i, c)
    elif ti.c_name == "double":
        c_check = "double_check(a%d, %s);" % (i, c)
    else:
        c_check = "if (a%d != %s) abort();" % (i, c)
    return q_arg, c_check


def main():
    for reps in range(1000):
        sys.stdout.write("\033[2K\r%d/%d" % (reps, 1000))
        sys.stdout.flush()
        c = open("fuzz_c.c", "w", newline="\n")
        q = open("fuzz_qbe.ssa", "w", newline="\n")

        ret_type = pick_type(VOID_ALLOWED)
        c_ret, q_call, q_cmp = fill_ret_value_c_check_qbe(ret_type)

        c.write("#include <math.h>\n")
        c.write("#include <stdint.h>\n")
        c.write("#include <stdio.h>\n")
        c.write("#include <stdlib.h>\n")
        c.write(
            "void float_check(float a, float b) { if (fabsf(a-b) > 0.00001) abort(); }\n"
        )
        c.write(
            "void double_check(double a, double b) { if (fabs(a-b) > 0.00001) abort(); }\n"
        )
        c.write("%s func_to_check(" % ret_type.c_name)
        q.write("export function w $main() {\n@start\n")
        q.write(q_call)

        num_args = random.randrange(0, 16)
        args = []
        cbody = []
        for i in range(num_args):
            arg = pick_type(NON_VOID)
            q_arg, c_check = call_from_qbe_check_in_c(i, arg)
            q.write(q_arg)
            if i < num_args - 1:
                q.write(", ")
            cbody.append(c_check)
            args.append("%s a%d" % (arg.c_name, i))

        argsstr = ", ".join(args)
        c.write(argsstr if argsstr else "void")
        c.write(") {\n")
        c.write("\n".join(cbody) + "\n")
        c.write('printf("ok!\\n");\n')

        q.write(")\n")

        c.write(c_ret)
        c.write("\n}\n")

        q.write(q_cmp)

        q.write("  ret 0\n")
        q.write("}\n")

        c.close()
        q.close()

        if sys.platform == "win32":
            QBE_PATH = ["qbe\\qbe.exe", "-t", "amd64_win"]
            CLANG_PATH = ["C:\\Program Files\\LLVM\\bin\\clang.exe"]
        else:
            QBE_PATH = ["./qbe/qbe"]
            CLANG_PATH = ["clang"]
        subprocess.run(QBE_PATH + ["-o", "fuzz_qbe.s", "fuzz_qbe.ssa"], check=True)
        subprocess.run(
            CLANG_PATH + ["fuzz_qbe.s", "fuzz_c.c", "-g", "-o", "fuzz_bin.exe"],
            check=True,
        )
        proc = subprocess.run(["./fuzz_bin.exe"], capture_output=True)
        if proc.returncode != 0:
            print("FAILED rc=%d\n---\n\n" % proc.returncode)
            subprocess.run(["cat", "fuzz_qbe.ssa"])
            print("\n---\n")
            subprocess.run(["cat", "fuzz_c.c"])
            sys.exit(1)


if __name__ == "__main__":
    main()
