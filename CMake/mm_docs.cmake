find_package(Doxygen)

if(DOXYGEN_FOUND)
	add_custom_command(
		OUTPUT ${CMAKE_BINARY_DIR}/documentation/index.html
			${CMAKE_BINARY_DIR}/latex/Makefile
		COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile
     		#DEPENDS  ${headers}
	)
	add_custom_command(
		OUTPUT ${CMAKE_BINARY_DIR}/documentation.pdf
		DEPENDS ${CMAKE_BINARY_DIR}/latex/Makefile
		COMMAND make
		COMMAND cmake -E copy refman.pdf ${CMAKE_BINARY_DIR}/documentation.pdf
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/latex
     		#DEPENDS  ${headers}
	)
      	add_custom_target(doc
	    	DEPENDS ${CMAKE_BINARY_DIR}/documentation/index.html
			${CMAKE_BINARY_DIR}/documentation.pdf
	    	COMMENT "Building doxygen documentation"
	)
      	add_custom_target(html
	    	DEPENDS ${CMAKE_BINARY_DIR}/documentation/index.html
	    	COMMENT "Building doxygen documentation"
	)
      	add_custom_target(pdf
		DEPENDS ${CMAKE_BINARY_DIR}/documentation.pdf
	    	COMMENT "Building doxygen documentation"
	)
	if(INCLUDE_DOCS)
	      	install(DIRECTORY ${CMAKE_BINARY_DIR}/documentation
	      		DESTINATION share/doc/mastermind/
		)
	endif(INCLUDE_DOCS)
endif()

add_custom_target(manual
	DEPENDS ${CMAKE_BINARY_DIR}/manual.pdf
		${CMAKE_BINARY_DIR}/manual/index.html
		${CMAKE_BINARY_DIR}/manual.txt
	COMMENT "Building Manual files"
)
add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/manual.pdf
	COMMAND texi2dvi --pdf ${CMAKE_CURRENT_SOURCE_DIR}/doc/mastermind.texi --output ${CMAKE_BINARY_DIR}/manual.pdf
	DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/doc/mastermind.texi
)
add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/manual.txt
	COMMAND makeinfo --plaintext ${CMAKE_CURRENT_SOURCE_DIR}/doc/mastermind.texi --output ${CMAKE_BINARY_DIR}/manual.txt
	DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/doc/mastermind.texi
)
add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/manual/index.html
	COMMAND makeinfo --html ${CMAKE_CURRENT_SOURCE_DIR}/doc/mastermind.texi --output ${CMAKE_BINARY_DIR}/manual/
	DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/doc/mastermind.texi
)
