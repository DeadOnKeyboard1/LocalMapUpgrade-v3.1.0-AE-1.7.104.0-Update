# Reproducible CommonLibSSE-NG source build for the Skyrim 1.7.104.0 update.
vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/alandtse/CommonLibSSE-NG.git
    REF 736dc64094e59232abfbcdf796cd0a063e136ec6
)

# CommonLibSSE-NG keeps OpenVR as a git submodule. vcpkg_from_git does not
# initialize submodules, so materialize the exact submodule revision explicitly.
vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH_OPENVR
    URL https://github.com/ValveSoftware/openvr.git
    REF 60eb187801956ad277f1cae6680e3a410ee0873b
)
file(REMOVE_RECURSE "${SOURCE_PATH}/extern/openvr")
file(COPY "${SOURCE_PATH_OPENVR}/" DESTINATION "${SOURCE_PATH}/extern/openvr")

# CommonLibSSE-NG 9.0.1 exports Microsoft::DirectXTK from its public link
# interface, but its installed Config.cmake only declares spdlog as a
# dependency.  Consumers then fail during CMake generation with
# "Microsoft::DirectXTK target was not found".  Fix the upstream package
# template before configuring so the generated vcpkg package is self-contained.
vcpkg_replace_string(
    "${SOURCE_PATH}/cmake/config.cmake.in"
    "find_dependency(spdlog CONFIG)"
    "find_dependency(spdlog CONFIG)\nfind_dependency(directxtk CONFIG)"
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTS=OFF
        -DENABLE_SKYRIM_SE=ON
        -DENABLE_SKYRIM_AE=ON
        -DENABLE_SKYRIM_VR=OFF
        -DSKSE_SUPPORT_XBYAK=ON
        -DSKSE_SUPPORT_PATCH_SAFETY=OFF
        -DCOMMONLIB_ENABLE_IPO=ON
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(
    PACKAGE_NAME CommonLibSSE
    CONFIG_PATH lib/cmake/CommonLibSSE
)
vcpkg_copy_pdbs()

# add_commonlibsse_plugin() is a consumer helper and is not part of the upstream
# install rule, therefore install it alongside the generated package config.
file(INSTALL "${SOURCE_PATH}/cmake/CommonLibSSE.cmake"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
# CommonLibSSE-NG 9.x stores the GPL text in COPYING.txt and its custom
# Skyrim/SKSE linking exceptions in EXCEPTIONS.md.  vcpkg requires a
# share/<port>/copyright file, so install COPYING.txt under that canonical name
# and keep the exception terms alongside it.
file(INSTALL "${SOURCE_PATH}/COPYING.txt"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
     RENAME copyright)
file(INSTALL "${SOURCE_PATH}/EXCEPTIONS.md"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
