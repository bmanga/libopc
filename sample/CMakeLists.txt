link_libraries(opc2)

FILE(GLOB sources *.c)

foreach(source ${sources})
	get_filename_component(program_name "${source}" NAME_WE)
	add_executable(${program_name} "${source}")
endforeach(source)
