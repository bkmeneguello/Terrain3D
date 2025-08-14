import os
from SCons.Script import AddOption, GetOption

Import('env')

AddOption("--libtiff-prefix", dest="libtiff_prefix", type="string",
          help="Root prefix to libtiff (contains include/ and lib/)")

def enable_libtiff(env):
    prefix = GetOption("libtiff_prefix")
    # 1. Explicit prefix
    if prefix:
        inc = os.path.join(prefix, "include")
        lib = os.path.join(prefix, "lib")
        if os.path.exists(os.path.join(inc, "tiffio.h")):
            env.Append(CPPPATH=[inc], LIBPATH=[lib], LIBS=["tiff"], CPPDEFINES=["HAVE_LIBTIFF"])
            print(f"libtiff: using prefix {prefix}")
            return True
        print(f"libtiff: tiffio.h not found under {prefix}")
        return False
    # 2. vcpkg (Windows)
    if env["platform"] == "windows":
        vcpkg_root = os.environ.get("VCPKG_ROOT") or os.environ.get("VCPKG_INSTALLATION_ROOT")
        if vcpkg_root:
            arch = env.get("arch", "x86_64")
            triplet = "x64-windows" if "64" in arch else "x86-windows"
            inc = os.path.join(vcpkg_root, "installed", triplet, "include")
            lib = os.path.join(vcpkg_root, "installed", triplet, "lib")
            if os.path.exists(os.path.join(inc, "tiffio.h")):
                env.Append(CPPPATH=[inc], LIBPATH=[lib], LIBS=["tiff"], CPPDEFINES=["HAVE_LIBTIFF"])
                print(f"libtiff: found in vcpkg ({triplet})")
                return True
    # 3. Homebrew (macOS)
    if env["platform"] == "macos":
        for root in ("/opt/homebrew", "/usr/local"):
            inc = f"{root}/include"
            lib = f"{root}/lib"
            if os.path.exists(f"{inc}/tiffio.h"):
                env.Append(CPPPATH=[inc], LIBPATH=[lib], LIBS=["tiff"], CPPDEFINES=["HAVE_LIBTIFF"])
                print(f"libtiff: found in {root}")
                return True
    # 4. pkg-config (Unix/macOS, MSYS, or Windows with pkg-config)
    for pc in ("libtiff-4", "libtiff"):
        try:
            env.ParseConfig(f"pkg-config --cflags --libs {pc}")
            env.Append(CPPDEFINES=["HAVE_LIBTIFF"])
            print(f"libtiff: found via pkg-config ({pc})")
            return True
        except Exception:
            pass
    return False

if enable_libtiff(env):
    print("TIFF support enabled (libtiff detected).")
else:
    print("WARNING: --with-tiff specified but libtiff not found; TIFF support disabled.")
