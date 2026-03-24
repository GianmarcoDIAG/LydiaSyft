# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/vboxuser/LydiaSyft/build/_deps/z3-src"
  "/home/vboxuser/LydiaSyft/build/_deps/z3-build"
  "/home/vboxuser/LydiaSyft/build/_deps/z3-subbuild/z3-populate-prefix"
  "/home/vboxuser/LydiaSyft/build/_deps/z3-subbuild/z3-populate-prefix/tmp"
  "/home/vboxuser/LydiaSyft/build/_deps/z3-subbuild/z3-populate-prefix/src/z3-populate-stamp"
  "/home/vboxuser/LydiaSyft/build/_deps/z3-subbuild/z3-populate-prefix/src"
  "/home/vboxuser/LydiaSyft/build/_deps/z3-subbuild/z3-populate-prefix/src/z3-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/vboxuser/LydiaSyft/build/_deps/z3-subbuild/z3-populate-prefix/src/z3-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/vboxuser/LydiaSyft/build/_deps/z3-subbuild/z3-populate-prefix/src/z3-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
