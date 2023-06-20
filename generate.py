# -*- coding: utf-8 -*-

FIF_TEMPLATE = "R\"for_c++_include(\n%s\n)for_c++_include\""

HPP_TEMPLATE = """
#pragma once

#define DECL_FIF(S) const std::string S = std::string
#define DECL_IS(X) std::istringstream is_##X(FiftDecl::X)

namespace FiftDecl {
%s
}
"""

DECL_FIF_TEMPLATE = """
DECL_FIF(%s) (
    #include "%s"
);
"""

DECL_IS_TEMPLATE = "\nDECL_IS(%s);\n"

PROCESS_INCLUDE = [
    ("./third-party/ton/crypto/fift/lib/Fift.fif",    "./generated/lib/Fift.fif"),
    ("./third-party/ton/crypto/fift/lib/Asm.fif",     "./generated/lib/Asm.fif"),
    ("./third-party/ton/crypto/fift/lib/Color.fif",   "./generated/lib/Color.fif"),
    ("./third-party/ton/crypto/fift/lib/Lists.fif",  "./generated/lib/Lists.fif"),
    ("./third-party/ton/crypto/fift/lib/GetOpt.fif",  "./generated/lib/GetOpt.fif"),
    ("./third-party/ton/crypto/fift/lib/TonUtil.fif", "./generated/lib/TonUtil.fif"),

    ("./fif-scripts/export-cfg.fif", "./generated/fif-scripts/export-cfg.fif"),
    ("./fif-scripts/import-cfg.fif", "./generated/fif-scripts/import-cfg.fif"),
    ("./fif-scripts/extmsg-cfg.fif", "./generated/fif-scripts/extmsg-cfg.fif"),
    ("./fif-scripts/dataof-cfg.fif", "./generated/fif-scripts/dataof-cfg.fif"),
    ("./fif-scripts/extmsg-chk.fif", "./generated/fif-scripts/extmsg-chk.fif"),
    ("./fif-scripts/dataof-chk.fif", "./generated/fif-scripts/dataof-chk.fif")
]

def read_file(filename: str):
    with open(filename, "r") as file:
        return file.read()

def write_file(filename: str, data: str):
    with open(filename, "w") as file:
        file.write(data)

def main():
    decls = ""

    for (f, t) in PROCESS_INCLUDE:
        print(f"generating {f} -> {t}")

        text = read_file(f)
        text = text.replace("\"Lists.fif\" include", "")
        text = text.replace("namespace Asm", "")
        text = text.replace("Asm definitions", "")
        
        out = []
        for line in text.split('\n'):
            if line.startswith('// INCLUDE: '):
                path = line.replace('// INCLUDE: ', '')
                incl = read_file(path)
                out.append(incl)
                continue

            out.append(line)

        write_file(t, FIF_TEMPLATE % ('\n'.join(out)))

        decl_name = f.split("/")[-1].upper()
        decl_name = decl_name.replace(".", "_")
        decl_name = decl_name.replace("-", "_")

        decls += DECL_FIF_TEMPLATE % (decl_name, "." + t)
        decls += DECL_IS_TEMPLATE % decl_name

    write_file("./generated/generated.hpp", (HPP_TEMPLATE % decls))

if __name__ == "__main__":
    main()