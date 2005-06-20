#!/usr/bin/python

import re
import sys

from distutils.core import setup, Extension

MODULE_NAME_RE = "[A-Za-z_]+"

if __name__ == "__main__":

    build_arg_re = re.compile(r"^([A-Z_]+)= *(.*)")
    def split_args((build_args, argv), arg):
        m = build_arg_re.match(arg)
        if m:
            (k, v) = m.groups()
            build_args[k] = v
        else:
            argv += [arg]
        return (build_args, argv)

    (build_args, argv) = reduce(split_args, sys.argv[1:], ({}, []))
    sys.argv[1:] = argv
    extension_args = { "extra_compile_args": ["-Wall"] }
    if "INCLUDES" in build_args:
        extension_args["include_dirs"] = re.findall(r"-I([^ ]+)",
                                                    build_args["INCLUDES"])
    lib_args = build_args.get("LIBS", "")
    if lib_args:
        extension_args["library_dirs"] = re.findall(r"-L([^ ]+)", lib_args)
        extension_args["libraries"] = re.findall(r"-l([^ ]+)", lib_args)
    modules = re.findall("(%s).py" % MODULE_NAME_RE,
                         build_args.get("PY_MODS", ""))
    extensions = map(lambda modname: Extension(modname,
                                               [modname + ".c"],
                                               **extension_args),
                     re.findall("(%s).so" % MODULE_NAME_RE,
                                build_args.get("PY_EXT_MODS", "")))

    setup(py_modules = modules, ext_modules = extensions)
