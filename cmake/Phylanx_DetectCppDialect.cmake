# Copyright (c) 2007-2017 Hartmut Kaiser
# Copyright (c) 2011-2017 Thomas Heller
# Copyright (c) 2017 Anton Bikineev
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

macro(phylanx_detect_cpp_dialect_non_msvc)

  if(PHYLANX_WITH_CUDA AND NOT PHYLANX_WITH_CUDA_CLANG)
    set(CXX_FLAG -std=c++11)
    phylanx_info("C++ mode used: C++11")
  else()

    # Try -std=c++17 first
    if(PHYLANX_WITH_CXX17 OR NOT (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
                              AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17)))
      check_cxx_compiler_flag(-std=c++17 PHYLANX_WITH_CXX17)
    endif()

    if(PHYLANX_WITH_CXX17)
      set(CXX_FLAG -std=c++17)
      phylanx_info("C++ mode used: C++17")
    else()
      # ... otherwise try -std=c++1z
      if(PHYLANX_WITH_CXX1Z OR NOT (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
                                AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17)))
        check_cxx_compiler_flag(-std=c++1z PHYLANX_WITH_CXX1Z)
      endif()

      if(PHYLANX_WITH_CXX1Z)
        set(CXX_FLAG -std=c++1z)
        phylanx_info("C++ mode used: C++1z")
      else()
        # ... otherwise try -std=c++14
        if(PHYLANX_WITH_CXX14 OR NOT (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
                                  AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17)))
          check_cxx_compiler_flag(-std=c++14 PHYLANX_WITH_CXX14)
        endif()

        if(PHYLANX_WITH_CXX14)
          set(CXX_FLAG -std=c++14)
          phylanx_info("C++ mode used: C++14")
        else()
          # ... otherwise try -std=c++1y
          if(PHYLANX_WITH_CXX1Y OR NOT (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
                                    AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17)))
            check_cxx_compiler_flag(-std=c++1y PHYLANX_WITH_CXX1Y)
          endif()

          if(PHYLANX_WITH_CXX1Y)
            set(CXX_FLAG -std=c++1y)
            phylanx_info("C++ mode used: C++1y")
          else()
            # ... otherwise try -std=c++11
            check_cxx_compiler_flag(-std=c++11 PHYLANX_WITH_CXX11)
            if(PHYLANX_WITH_CXX11)
              set(CXX_FLAG -std=c++11)
              phylanx_info("C++ mode used: C++11")
            else()
              # ... otherwise try -std=c++0x
              check_cxx_compiler_flag(-std=c++0x PHYLANX_WITH_CXX0X)
              if(PHYLANX_WITH_CXX0X)
                set(CXX_FLAG -std=c++0x)
                phylanx_info("C++ mode used: C++0x")
              endif()
            endif()
          endif()
        endif()
      endif()
    endif()
  endif()
endmacro()

macro(phylanx_detect_cpp_dialect)

  if(MSVC)
    set(CXX_FLAG)

    # enable enforcing a particular C++ mode
    if(PHYLANX_WITH_CXX17)
      set(CXX_FLAG /std:c++17)
      phylanx_info("C++ mode enforced: C++17")
      phylanx_add_target_compile_option(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
      phylanx_add_target_compile_option(-D_HAS_AUTO_PTR_ETC=1)
    elseif(PHYLANX_WITH_CXX1Z)
      set(CXX_FLAG /std:c++latest)
      phylanx_info("C++ mode enforced: C++1z")
      phylanx_add_target_compile_option(-D_HAS_AUTO_PTR_ETC=1)
    elseif(PHYLANX_WITH_CXX14)
      set(CXX_FLAG /std:c++14)
      phylanx_info("C++ mode enforced: C++14")
      phylanx_add_target_compile_option(-D_HAS_AUTO_PTR_ETC=1)
    elseif(PHYLANX_WITH_CXX1Y)
      set(CXX_FLAG /std:c++14)
      phylanx_info("C++ mode enforced: C++1y")
    elseif(PHYLANX_WITH_CXX11)
      phylanx_info("C++ mode enforced: C++11")
    elseif(PHYLANX_WITH_CXX0X)
      phylanx_info("C++ mode enforced: C++0x")
    else()
      phylanx_info("C++ mode assumed: C++11")
    endif()

  else(MSVC)

    # enable enforcing a particular C++ mode
    if(PHYLANX_WITH_CXX17)
      set(CXX_FLAG -std=c++17)
      phylanx_info("C++ mode enforced: C++17")
    elseif(PHYLANX_WITH_CXX1Z)
      set(CXX_FLAG -std=c++1z)
      phylanx_info("C++ mode enforced: C++1z")
    elseif(PHYLANX_WITH_CXX14)
      set(CXX_FLAG -std=c++14)
      phylanx_info("C++ mode enforced: C++14")
    elseif(PHYLANX_WITH_CXX1Y)
      set(CXX_FLAG -std=c++1y)
      phylanx_info("C++ mode enforced: C++1y")
    elseif(PHYLANX_WITH_CXX11)
      set(CXX_FLAG -std=c++11)
      phylanx_info("C++ mode enforced: C++11")
    elseif(PHYLANX_WITH_CXX0X)
      set(CXX_FLAG -std=c++0x)
      phylanx_info("C++ mode enforced: C++0x")
    else()
      # if no C++ mode is enforced, try to detect which one to use
      phylanx_detect_cpp_dialect_non_msvc()
    endif()

  endif(MSVC)

  if(CXX_FLAG)
    phylanx_add_target_compile_option(${CXX_FLAG})
  endif()

endmacro()