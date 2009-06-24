#Copyright or © or Copr. CNRS, (June 24, 2009)
#
#This software is a computer program whose purpose is to provide classes
#for using the acnuc database query system.
#
#This software is governed by the CeCILL  license under French law and
#abiding by the rules of distribution of free software.  You can  use, 
#modify and/ or redistribute the software under the terms of the CeCILL
#license as circulated by CEA, CNRS and INRIA at the following URL
#"http://www.cecill.info". 
#
#As a counterpart to the access to the source code and  rights to copy,
#modify and redistribute granted by the license, users are provided only
#with a limited warranty  and the software's author,  the holder of the
#economic rights,  and the successive licensors  have only  limited
#liability. 
#
#In this respect, the user's attention is drawn to the risks associated
#with loading,  using,  modifying and/or developing or reproducing the
#software by the user in light of its specific status of free software,
#that may mean  that it is complicated to manipulate,  and  that  also
#therefore means  that it is reserved for developers  and  experienced
#professionals having in-depth computer knowledge. Users are therefore
#encouraged to load and test the software's suitability as regards their
#requirements in conditions enabling the security of their systems and/or 
#data to be ensured and,  more generally, to use and operate it in the 
#same conditions as regards security. 
#
#The fact that you are presently reading this means that you have had
#knowledge of the CeCILL license and that you accept its terms.

H_FILES = \
parser.h\
RAA.h\
RaaSeqAttributes.h\
RAA_acnuc.h\
RaaList.h\
RaaSpeciesTree.h


CPP_FILES = \
RAA.cpp\
RaaList.cpp\
RaaSpeciesTree.cpp

C_FILES = \
md5.c\
parser.c\
zsockr.c\
misc_acnuc.c\
RAA_acnuc.c

lib_LTLIBRARIES = libbpp-raa.la
libbpp_raa_la_SOURCES = $(H_FILES) $(CPP_FILES) $(C_FILES)
# CURRENT:REVISION:AGE => file.so.(C-A).A.R
# current: The most recent interface number that this library implements.
# revision: The implementation number of the current interface.
# age: The difference between the newest and oldest interfaces that this library implements.
# In other words, the library implements all the interface numbers in the range from number current - age to current.
libbpp_raa_la_LDFLAGS = -version-info 1:0:0

library_includedir=$(includedir)/Bpp/Raa
library_include_HEADERS = $(H_FILES)

