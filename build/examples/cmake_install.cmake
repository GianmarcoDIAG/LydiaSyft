# Install script for directory: /home/vboxuser/LydiaSyft/examples

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "RelWithDebInfo")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/vboxuser/LydiaSyft/build/examples/01_quickstart/cmake_install.cmake")
  include("/home/vboxuser/LydiaSyft/build/examples/02_dfa_representation/cmake_install.cmake")
  include("/home/vboxuser/LydiaSyft/build/examples/03_dfa_creation_and_manipulation/cmake_install.cmake")
  include("/home/vboxuser/LydiaSyft/build/examples/04_ltlf_synthesis/cmake_install.cmake")
  include("/home/vboxuser/LydiaSyft/build/examples/05_ltlf_synthesis_maximally_permissive/cmake_install.cmake")
  include("/home/vboxuser/LydiaSyft/build/examples/06_ltlf_synthesis_with_fairness_conditions/cmake_install.cmake")
  include("/home/vboxuser/LydiaSyft/build/examples/07_ltlf_synthesis_with_stability_conditions/cmake_install.cmake")
  include("/home/vboxuser/LydiaSyft/build/examples/08_ltlf_synthesis_with_gr1_env_spec/cmake_install.cmake")

endif()

