#
# This file is modeled after appleseed.
# Visit https://appleseedhq.net/ for additional information and resources.
#
# This software is released under the MIT license.
#
# Copyright (c) 2014-2018 Esteban Tovagliari, The appleseedhq Organization
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

###
# - Find Assimp
# Find the native Assimp includes and library
#
#  ASSIMP_INCLUDE_DIR   - where to find include headers
#  ASSIMP_LIBRARIES     - List of libraries when using assimp.
#
###
# - helpful cmake flags when building
#
#  Assimp_ROOT          - path to assimp root if it is built outside of /usr
#
#=============================================================================


include (FindPackageHandleStandardArgs)

find_path (Assimp_INCLUDE_DIR 
            NAMES assimp/postprocess.h assimp/scene.h assimp/version.h assimp/config.h assimp/cimport.h
	    PATHS /usr/local/include
	    PATHS /usr/include/
            HINTS 
            ${Assimp_ROOT}
	    PATH_SUFFIXES
            include
	)

find_library (Assimp_LIBRARY
  	    NAMES assimp
            PATHS /usr/local/lib/
	    PATHS /usr/lib64/
	    PATHS /usr/lib/
	    HINTS
	    ${Assimp_ROOT}
	    PATH_SUFFIXES
            bin
  	    lib
	    lib64
	)

# Handle the QUIETLY and REQUIRED arguments and set assimp_FOUND.
find_package_handle_standard_args (Assimp DEFAULT_MSG
    Assimp_INCLUDE_DIR
    Assimp_LIBRARY
)

# Set the output variables.
if (assimp_FOUND)
    set (Assimp_INCLUDE_DIRS ${Assimp_INCLUDE_DIR})
    set (Assimp_LIBRARIES ${Assimp_LIBRARY})
else ()
    set (Assimp_INCLUDE_DIRS)
    set (Assimp_LIBRARIES)
endif ()

mark_as_advanced (
    Assimp_INCLUDE_DIR
    Assimp_LIBRARY
)
