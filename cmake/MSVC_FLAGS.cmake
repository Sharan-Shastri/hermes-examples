# Always use inlining.
# Disable some warnings.
# Use multi-processor.
set(HERMES_MSVC_CXX_FLAGS "/Ob2 /wd4275 /wd4251 /wd4018 /wd4251 /wd4661 /wd4996 /MP")
if(WITH_OPENMP)
  SET(HERMES_MSVC_CXX_FLAGS "${HERMES_MSVC_CXX_FLAGS} /openmp")
endif(WITH_OPENMP)

if(WIN64)
  set(HERMES_MSVC_CXX_FLAGS "${HERMES_MSVC_CXX_FLAGS} /DWIN64")
endif(WIN64)

# Generate debug info in the proper format.
SET(HERMES_MSVC_DEBUG_INFO_FLAGS "/Zi /MDd")
# No debug info.
SET(HERMES_MSVC_NDEBUG_FLAGS "/DNDEBUG /MD")
# Optimizations.
SET(HERMES_MSVC_OPTIMIZATIONS_FLAGS "/O2 /Ot")
# NO optimizations.
SET(HERMES_MSVC_NO_OPTIMIZATIONS_FLAGS "/Od")

# Debugging - add the "_DEBUG" definition.
set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG ${HERMES_MSVC_DEBUG_INFO_FLAGS} ${HERMES_MSVC_NO_OPTIMIZATIONS_FLAGS} ${HERMES_MSVC_CXX_FLAGS}")
# Release with deb. info -> primarily for VTune etc.
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${HERMES_MSVC_DEBUG_INFO_FLAGS} ${HERMES_MSVC_OPTIMIZATIONS_FLAGS} ${HERMES_MSVC_CXX_FLAGS}")
# Release
set(CMAKE_CXX_FLAGS_RELEASE "${HERMES_MSVC_NDEBUG_FLAGS} ${HERMES_MSVC_OPTIMIZATIONS_FLAGS} ${HERMES_MSVC_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_RELEASE}")

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")