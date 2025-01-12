import os
import platform
import re
import string
import subprocess
import sys

LIBQBE_C_FILES = [
    "all.h",
    "amd64/all.h",
    "arm64/all.h",
    "rv64/all.h",
    "abi.c",
    "alias.c",
    "cfg.c",
    "copy.c",
    "emit.c",
    "fold.c",
    "live.c",
    "load.c",
    "main.c",
    "mem.c",
    "parse.c",
    "rega.c",
    "simpl.c",
    "spill.c",
    "ssa.c",
    "util.c",
    "amd64/emit.c",
    "amd64/isel.c",
    "amd64/sysv.c",
    "amd64/targ.c",
    "arm64/abi.c",
    "arm64/emit.c",
    "arm64/isel.c",
    "arm64/targ.c",
    "rv64/abi.c",
    "rv64/emit.c",
    "rv64/isel.c",
    "rv64/targ.c",
    "../libqbe_impl.c",
]


def namespace_static_funcs(ns, file, contents):
    lines = contents.splitlines()
    function_names = []
    for i, line in enumerate(lines):
        if (
            i < len(lines) - 1
            and line.startswith("static ")
            and lines[i + 1]
            and not lines[i + 1].startswith("static")
            and lines[i + 1][0] in string.ascii_lowercase
        ):
            fn = lines[i + 1].partition("(")[0]
            if fn == "amd64_memargs" or fn == "arm64_memargs" or fn == "rv64_memargs":
                continue
            function_names.append(fn)
    for fn in function_names:
        contents = re.sub(r"\b" + fn + "\\(", ns + fn + "(", contents)
        contents = re.sub(
            r"qsort\((.*) " + fn + "\);", r"qsort(\1 " + ns + fn + ");", contents
        )
        contents = re.sub(
            r"loopiter\((.*) " + fn + "\);", r"loopiter(\1 " + ns + fn + ");", contents
        )
        if file == "main.c":
            contents = re.sub(
                r"parse\((.*)\b" + fn + r"\b", r"parse(\1" + ns + fn, contents
            )

    return contents


def emit_renames(ns, contents):
    ns_up = ns.upper()
    for x in ["E", "Ki", "Ka"]:
        contents = re.sub(r"\b" + x + r"\b", ns_up + x, contents)
    for x in ["omap", "rname"]:
        contents = re.sub(r"\b" + x + r"\b", ns + x, contents)
    return contents + "\n#undef CMP\n\n"


def abi_renames(ns, contents):
    ns_up = ns.upper()
    for x in ["Cstk", "Cptr", "Cstk1", "Cstk2", "Cfpint", "Class", "Insl", "Params"]:
        contents = re.sub(r"\b" + x + r"\b", ns_up + x, contents)
    for x in ["gpreg", "fpreg"]:
        contents = re.sub(r"\b" + x + r"\b", ns + x, contents)
    return contents


def arm64_reg_rename(contents):
    regs = (
        ["R%d" % i for i in range(16)]
        + ["IP0", "IP1"]
        + ["R%d" % i for i in range(18, 29)]
        + ["V%d" % i for i in range(31)]
        + ["FP", "LR", "SP", "NFPR", "NGPR", "NGPS", "NFPS", "NCLR"]
    )
    for i in regs:
        contents = re.sub(r"\b%s\b" % i, "QBE_ARM64_%s" % i, contents)
    return contents


def amd64_reg_rename(contents):
    regs = (
        [
            "RAX",
            "RCX",
            "RDX",
            "RSI",
            "RDI",
            "R8",
            "R9",
            "R10",
            "R11",
            "RBX",
            "R12",
            "R13",
            "R14",
            "R15",
            "RBP",
            "RSP",
        ]
        + ["XMM%d" % i for i in range(16)]
        + ["NFPR", "NGPR", "NGPS", "NFPS", "NCLR", "RGLOB"]
    )
    for i in regs:
        contents = re.sub(r"\b%s\b" % i, "QBE_AMD64_%s" % i, contents)
    return contents


def rv64_reg_rename(contents):
    regs = (
        ["T%d" % i for i in range(7)]
        + ["A%d" % i for i in range(8)]
        + ["S%d" % i for i in range(12)]
        + ["FT%d" % i for i in range(12)]
        + ["FA%d" % i for i in range(8)]
        + ["FS%d" % i for i in range(12)]
        + [
            "FP",
            "SP",
            "GP",
            "TP",
            "RA",
            "NFPR",
            "NGPR",
            "NGPS",
            "NFPS",
            "NCLR",
            "RGLOB",
        ]
    )
    for i in regs:
        contents = re.sub(r"\b%s\b" % i, "QBE_RV64_%s" % i, contents)
    return contents


def make_instr_prototypes(ops_h_contents):
    external_only = []
    for x in ops_h_contents.splitlines():
        if "INTERNAL OPERATIONS" in x:
            break
        if x.startswith("O("):
            external_only.append(x)

    ret = ""
    for op in external_only:
        assert op.startswith("O(")
        toks = re.split("[ \t,T()]+", op[2:])
        op = toks[0]
        lhs = "".join(toks[1:5])
        rhs = "".join(toks[5:9])
        if rhs == "xxxx":
            ret += "void lq_i_%s(LqRef lhs /*%s*/);\n" % (op, "".join(lhs))
        else:
            ret += "void lq_i_%s(LqRef lhs /*%s*/, LqRef rhs /*%s*/);\n" % (
                op,
                "".join(lhs),
                "".join(rhs),
            )

    return ret


def get_config():
    # TODO, match Makefile, and I think put at runtime instead
    if platform.machine().lower() == "arm64":
        return "#define Deftgt T_arm64_apple"
    else:
        return "#define Deftgt T_amd64_sysv"


def main():
    QBE_ROOT = os.path.join(os.getcwd(), "qbe")
    if not os.path.exists(QBE_ROOT):
        subprocess.call(["git", "clone", "git://c9x.me/qbe.git"])

    with open(os.path.join(QBE_ROOT, "ops.h"), "r") as f:
        ops_h_contents = f.read()
    with open(os.path.join(QBE_ROOT, "LICENSE"), "r") as f:
        license_contents = f.read()
    with open("libqbe.c", "w", newline="\n") as amalg:
        amalg.write("/*\n\nQBE LICENSE:\n\n")
        amalg.write(license_contents)
        amalg.write("\n---\n\n")
        amalg.write("Other libqbe code under the same license by Scott Graham.\n\n")
        amalg.write("*/\n\n")
        amalg.write(get_config())
        amalg.write("\n")

        for file in LIBQBE_C_FILES:
            with open(os.path.join(QBE_ROOT, file), "rb") as f:
                contents = f.read().decode("utf-8")

            ns = "qbe_" + file.replace("/", "_").replace(".c", "") + "_"

            if file.endswith(".c"):
                contents = namespace_static_funcs(ns, file, contents)

            if file == "arm64/all.h" or file.startswith("arm64/"):
                contents = arm64_reg_rename(contents)

            if file == "amd64/all.h" or file.startswith("amd64/"):
                contents = amd64_reg_rename(contents)

            if file == "rv64/all.h" or file.startswith("rv64/"):
                contents = rv64_reg_rename(contents)

            if file.endswith("/emit.c"):
                contents = emit_renames(ns, contents)

            if file.endswith("/abi.c"):
                contents = abi_renames(ns, contents)

            amalg.write("/*** START FILE: %s ***/\n" % file)
            for line in contents.splitlines():
                if line.startswith('#include "all.h"'):
                    amalg.write("/* skipping all.h */\n")
                    continue
                if line.startswith('#include "../all.h"'):
                    amalg.write("/* skipping ../all.h */\n")
                    continue
                if line.startswith('#include "config.h"'):
                    amalg.write("/* skipping config.h */\n")
                    continue
                if line.startswith("#include <getopt.h>"):
                    amalg.write('#include "getopt.h"\n')
                    continue
                if line.strip().startswith(
                    '#include "ops.h"'
                ) or line.strip().startswith('#include "../ops.h"'):
                    amalg.write("/* " + 60 * "-" + "including ops.h */\n")
                    amalg.write(ops_h_contents)
                    amalg.write("/* " + 60 * "-" + "end of ops.h */\n")
                    continue
                amalg.write(line)
                amalg.write("\n")
            amalg.write("/*** END FILE: %s ***/\n" % file)

    with open("libqbe.in.h", "r", newline="\n") as header_in:
        header_contents = header_in.read()

    header_contents = header_contents.replace(
        "%%%INSTRUCTION_DECLARATIONS%%%\n", make_instr_prototypes(ops_h_contents)
    )

    with open("libqbe.h", "w", newline="\n") as header_out:
        header_out.write(header_contents)


if __name__ == "__main__":
    main()
