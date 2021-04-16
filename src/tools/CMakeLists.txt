foreach (tool ${TOOLS})
	message (STATUS "Include tool ${tool}")

	if (${tool} STREQUAL "web")
		add_subdirectory ("elektrad")
		add_subdirectory ("webd")
		add_subdirectory ("webui")
	else ()
		add_subdirectory ("${tool}")
	endif (${tool} STREQUAL "web")

endforeach (tool)
