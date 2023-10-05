###############################################################################
# CUTOMIZE THIS FILE TO SUIT YOUR NEEDS FOR THE BUILD PROCESS                 #
# This file is included into the following files (refer to them for more      #
# information on the build process):                                          #
#  - Makefile                                                                 #
###############################################################################

# The version number of the box library ($(VMAJ).$(VMIN).$(VREV))
VMAJ = 0
VMIN = 10
VREV = 2
VDEB = 1

# the RTS library
LIB_SMXZLOG = -lsmxzlog-0.3

# the include path of the smxzlog header files
INC_SMXZLOG = -I/usr/include/smx/libsmxzlog-0.3

# the name of the library
LIBNAME = smxrts
