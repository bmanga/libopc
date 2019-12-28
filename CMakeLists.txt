cmake_minimum_required(VERSION 3.8)

project(libopc-v2)

find_package(LibXml2)
find_package(ZLIB)

add_library(opc2
	config.h
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

target_include_directories(opc2 PUBLIC . ${LIBXML2_INCLUDE_DIR} opc/)
target_link_libraries(opc2 PRIVATE ${LIBXML2_LIBRARIES} ZLIB::ZLIB)

option (ENABLE_SAMPLES "Enable all targets in the sample/ folder" False)
if(ENABLE_SAMPLES)
	add_subdirectory(sample)
endif()