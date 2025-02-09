cmake_minimum_required(VERSION 3.8)

project(libopc-v2 C CXX)

find_package(LibXml2)
find_package(ZLIB)

add_library(opc2
	opc/config.h
	opc/container.c
	opc/container.h
	opc/file.c
	opc/file.h
	opc/helper.c
	opc/helper.h
	opc/inputstream.c
	opc/inputstream.h
	opc/internal.h
	opc/main.c
	opc/opc.c
	opc/opc.h
	opc/outputstream.c
	opc/outputstream.h
	opc/part.c
	opc/part.h
	opc/properties.c
	opc/properties.h
	opc/relation.c
	opc/relation.h
	opc/xmlreader.c
	opc/xmlreader.h
	opc/xmlwriter.c
	opc/xmlwriter.h
	opc/zip.c
	opc/zip.h
  opc/mce/helper.c
  opc/mce/helper.h
  opc/mce/textreader.c
  opc/mce/textreader.h
  opc/mce/textwriter.c
  opc/mce/textwriter.h)

target_include_directories(opc2 PUBLIC . ${LIBXML2_INCLUDE_DIR})
target_link_libraries(opc2 PRIVATE ${LIBXML2_LIBRARIES} ZLIB::ZLIB)

add_library(cppopc2
	opc++/opc.hpp
	opc++/opc.cpp opc++/container.hpp opc++/container.cpp)

target_link_libraries(cppopc2 PRIVATE opc2)
target_include_directories(cppopc2 PUBLIC . ${LIBXML2_INCLUDE_DIR})

option (ENABLE_SAMPLES "Enable all targets in the sample/ folder" False)
if(ENABLE_SAMPLES)
	add_subdirectory(sample)
	add_subdirectory(sample++)
endif()