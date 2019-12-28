link_libraries(cppopc2)

FILE(GLOB sources *.cpp)

foreach(source ${sources})
	get_filename_component(program_name "${source}" NAME_WE)
	add_executable(cpp_${program_name} "${source}")
endforeach(source)
