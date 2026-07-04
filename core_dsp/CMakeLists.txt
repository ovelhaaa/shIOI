cmake_minimum_required(VERSION 3.15)

# core_dsp is an INTERFACE library because it is heavily templated/inline-based
# and we want it to be header-only. This avoids "multiple definition" linker errors
# when included in different platform builds.
add_library(core_dsp INTERFACE)

# Define the include directories that consumers of core_dsp will need.
# Using INTERFACE here ensures that any target that links to core_dsp
# will automatically get this directory added to its include path.
target_include_directories(core_dsp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Note: Platforms should link to core_dsp like this:
# target_link_libraries(YourPlatformTarget PRIVATE core_dsp)
# 
# And they should define their specific flags (e.g. -DPLATFORM_PICO) 
# in their own CMakeLists.txt via:
# target_compile_definitions(YourPlatformTarget PRIVATE PLATFORM_PICO)
