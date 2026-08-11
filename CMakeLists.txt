cmake_minimum_required(VERSION 3.15...4.3)

project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION} LANGUAGES C)

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module
                                        ${SKBUILD_SABI_COMPONENT})

add_definitions(-Wno-format-truncation -Wincompatible-pointer-types)
find_package(HDF5 REQUIRED)

# NGPD REQUIRES FOLLOWING LIBS
# libs = ['ngpd_noadq14', 'ngpd_cal', 'img_mod', 'hdf5', 'z', 'dl', 'rt']
# LIBDIR IN DET_SOFTWARE:
# libs/libs.linux.x86_64/lib/
#INCLUDE DIRS
# none_vme/ngpd/include libs/include

set(NGPD_SRC_FILES
  lib/ngpd/ngpd_adq14_adc.c
  lib/ngpd/ngpd_adq14_dma.c
  lib/ngpd/ngpd_adq14_regs.c
  lib/ngpd/ngpd_adq14_scope_regs.c
  lib/ngpd/ngpd_adq14.c
  lib/ngpd/ngpd_filter.c
  lib/ngpd/ngpd_histogram.c
  lib/ngpd/ngpd_playback.c
  lib/ngpd/ngpd_save_restore.c
  lib/ngpd/ngpd_scope_mode.c
  lib/ngpd/ngpd_zynqmp.c
  lib/ngpd/ngzmp_adc.c
  lib/ngpd/ngzmp_ams.c
  lib/ngpd/ngzmp_clock.c
  lib/ngpd/ngzmp_dma.c
  lib/ngpd/ngzmp_hardware_conf.c
  lib/ngpd/ngzmp_hist.c
  lib/ngpd/ngzmp_i2c.c
  lib/ngpd/ngzmp_spi.c
  lib/zynqmp/zynqmp_api.c
)


set(NGPD_CDEF "${CMAKE_CURRENT_SOURCE_DIR}/src/ngpd/lib/pyngpd.h")
set(NGPD_CSRC "${CMAKE_CURRENT_SOURCE_DIR}/src/ngpd/lib/pyngpd.csrc.c")

# IMG_MOD OBJECTS LIBRARY~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
add_library(img_mod OBJECT lib/img_mod/img_mod.c lib/img_mod/img_mod_linux.c)
target_include_directories(img_mod PRIVATE lib/include)
target_compile_options(img_mod PRIVATE -D__LINUX__=1 -fPIC)

# NGPD Static Library~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
add_library(_ngpd STATIC
  $<TARGET_OBJECTS:img_mod>
  ${NGPD_SRC_FILES}
)

target_include_directories(_ngpd PUBLIC
  lib/include
  ${HDF5_INCLUDE_DIRS}
)

target_link_libraries(_ngpd PUBLIC
  ${HDF5_LIBRARIES}
)

target_compile_options(_ngpd PUBLIC -fPIC -O2 -fno-omit-frame-pointer)
install(TARGETS _ngpd DESTINATION ngpd)


add_custom_command(
  OUTPUT _pyngpd.c
  COMMAND Python::Interpreter -m cffi.gen_src read-sources ngpd._core
  "${NGPD_CDEF}" "${NGPD_CSRC}" _pyngpd.c
  DEPENDS "${NGPD_CDEF}" "${NGPD_CSRC}"
  VERBATIM)

# cffi's generated code targets the limited API unless
# _CFFI_NO_LIMITED_API is defined (or unavailable, like on 3.14t)
if(NOT "${SKBUILD_SABI_COMPONENT}" STREQUAL "")
  python_add_library(_core MODULE USE_SABI "${SKBUILD_SABI_VERSION}" WITH_SOABI
                     "${CMAKE_CURRENT_BINARY_DIR}/_pyngpd.c" ${NGPD_SRC_FILES})
else()
  python_add_library(_core MODULE WITH_SOABI
                     "${CMAKE_CURRENT_BINARY_DIR}/_pyngpd.c" ${NGPD_SRC_FILES})
endif()
target_include_directories(_core PRIVATE lib)
target_link_libraries(_core PUBLIC _ngpd)

install(TARGETS _core DESTINATION ngpd)