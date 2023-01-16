from cffi import FFI
platform="x86_64"
#platform="w64"
if platform=="x86_64" :
    # libdirs = ['../../../libs/libs.linux.x86_64/lib/']
    # libs = ['ngpd_noadq14','img_mod', 'rt']
    libdirs = ['../../install/lib']
    libs = ['ngpd', 'img_mod', 'adq', 'rt']
elif platform=="mgw64" :
    libdirs = ['../../../libs/libs.mgw64/lib/']
    libs = ['ngpd_noadq14','img_mod', 'ws2_32' ]
elif platform=="w64" :
    libdirs = ['../../../libs/libs.w64/lib/']
    libs = ['ngpd_noadq14','img_mod', 'pthreadVC2',  'ws2_32' ]
else:
    libdirs = ['../../../libs/libs.w64/lib/']
    libs = ['libngpd_noadq14.a','libimg_mod.a','ws2_32' ]
ffibuilder = FFI()
ffibuilder.set_source("pyngpd_cffi",
    r""" // passed to the real C compiler,
    #include "ngpd.h"
    #include "ngpd_dummy.h"
    """,
    libraries=libs,   # or a list of libraries to link with
    # (more arguments likse setup.py's Extension class:
    include_dirs=['../include', '../../install/include', '../../det_sw_include'], library_dirs=libdirs, extra_compile_args=["-D__LINUX__"])

with open("pyngpd.h", "r") as header:
    header_info = header.read()


ffibuilder.cdef(header_info)



if __name__ == "__main__":
    ffibuilder.compile(verbose=True, debug=False)
