# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/kfarrell/PROGRAMMING/ChatCurses/_deps/nlohmann_json-src")
  file(MAKE_DIRECTORY "/home/kfarrell/PROGRAMMING/ChatCurses/_deps/nlohmann_json-src")
endif()
file(MAKE_DIRECTORY
  "/home/kfarrell/PROGRAMMING/ChatCurses/_deps/nlohmann_json-build"
  "/home/kfarrell/PROGRAMMING/ChatCurses/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix"
  "/home/kfarrell/PROGRAMMING/ChatCurses/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/tmp"
  "/home/kfarrell/PROGRAMMING/ChatCurses/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp"
  "/home/kfarrell/PROGRAMMING/ChatCurses/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/src"
  "/home/kfarrell/PROGRAMMING/ChatCurses/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/kfarrell/PROGRAMMING/ChatCurses/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/kfarrell/PROGRAMMING/ChatCurses/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
