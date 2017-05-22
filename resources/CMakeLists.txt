IF(${WIN32})
set(NASM ${DEPENDENCY_DIR}/nasm/nasm.exe)

function(preprocess_resources out_var objs_var)
  set(result)
  set(objs)

  foreach(in_f ${ARGN})
    file(RELATIVE_PATH out_f ${CMAKE_CURRENT_SOURCE_DIR} ${in_f})
    get_filename_component(raw_name ${in_f} NAME)
    get_filename_component(rel_dir ${out_f} DIRECTORY)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${rel_dir})
    string(REGEX REPLACE "[/.-]" "_" name ${raw_name})
    string(REPLACE "/" "\\" escaped_fname ${in_f})
    set(out_f "${CMAKE_CURRENT_BINARY_DIR}/${out_f}")
    file(RELATIVE_PATH rel_fn ${CMAKE_CURRENT_SOURCE_DIR}/templates/ ${in_f})
    string(REGEX REPLACE "[/.-]" "_" rel_escaped ${rel_fn})

    add_custom_command(OUTPUT ${out_f}.obj
     rsrc.hpp
     externs.hpp
     COMMAND PowerShell -Command "$x=Get-Content ${CMAKE_CURRENT_SOURCE_DIR}/win64-data.asm; $x=$x.Replace('<name>', '${rel_escaped}'); $x.Replace('<bin>', '${escaped_fname}') | set-content -path ${out_f}.asm -encoding 'ASCII'"
     COMMAND ${NASM} -f win64 ${out_f}.asm -o ${out_f}.obj
     COMMAND PowerShell -Command "$x=Get-Content ${CMAKE_CURRENT_SOURCE_DIR}/resource_line_template.txt; $x=$x.Replace('<fn>', '${rel_fn}'); $x.Replace('<name>', '${rel_escaped}');" >> ${CMAKE_CURRENT_SOURCE_DIR}/rsrc.hpp
     COMMAND PowerShell -Command "echo 'extern int ${rel_escaped}_start;'" >> ${CMAKE_CURRENT_SOURCE_DIR}/externs.hpp
     COMMAND PowerShell -Command "echo 'extern int ${rel_escaped}_size;'" >> ${CMAKE_CURRENT_SOURCE_DIR}/externs.hpp
     DEPENDS ${in_f}
     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
     COMMENT "${NASM} -f win64 ${out_f}.asm -o ${out_f}.obj"
     VERBATIM
    )
    list(APPEND result ${out_f})
    list(APPEND objs "${out_f}.obj")
  endforeach()
  set(${out_var} "${result}" PARENT_SCOPE)
  set(${objs_var} "${objs}" PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE files ${CMAKE_CURRENT_SOURCE_DIR}/templates/ *.html *.png *.css *.js)
preprocess_resources(files rsrc_objs ${files})

add_library(resources
resource.cpp
resource.hpp
rsrc.cpp
${CMAKE_CURRENT_SOURCE_DIR}/rsrc.hpp
${CMAKE_CURRENT_SOURCE_DIR}/externs.hpp
${rsrc_objs})

file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/externs.hpp EXTERNSFILE)
file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/rsrc.hpp RSRCFILE)

set_target_properties(resources PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(resources
PUBLIC
$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

ELSE()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  SET(NASM nasm)
  SET(NASMFLAGS "-f elf64")
  SET(NASMTEMPLATE "linux.asm")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(NASM nasm)
  SET(NASMFLAGS "-f macho64")
  SET(NASMTEMPLATE "data.asm")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  SET(NASM nasm)
  SET(NASMFLAGS "-f elf64")
  SET(NASMTEMPLATE "linux.asm")
ELSE()
ENDIF()

find_program(SED_EXECUTABLE sed)

function(preprocess_resources out_var objs_var)
  set(result)
  set(objs)

  foreach(in_f ${ARGN})
    file(RELATIVE_PATH out_f ${CMAKE_CURRENT_SOURCE_DIR} ${in_f})
    get_filename_component(raw_name ${in_f} NAME)
    get_filename_component(rel_dir ${out_f} DIRECTORY)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${rel_dir})

    string(REGEX REPLACE "[/.-]" "_" name ${raw_name})
    string(REPLACE "/" "\\/" escaped_fname ${in_f})
    set(out_f "${CMAKE_CURRENT_BINARY_DIR}/${out_f}")
    file(RELATIVE_PATH rel_fn ${CMAKE_CURRENT_SOURCE_DIR}/templates/ ${in_f})
    string(REGEX REPLACE "[/.-]" "_" rel_escaped ${rel_fn})

    add_custom_command(OUTPUT ${out_f}.o
    rsrc.hpp
    externs.hpp
    COMMAND ${SED_EXECUTABLE} -e s/<name>/${rel_escaped}/g; -e s/<bin>/${escaped_fname}/g; ${CMAKE_CURRENT_SOURCE_DIR}/${NASMTEMPLATE} > ${out_f}.asm
      COMMAND echo "{\"/${rel_fn}\", { ${rel_escaped}_size, &${rel_escaped}_start} }," >> ${CMAKE_CURRENT_SOURCE_DIR}/rsrc.hpp
      COMMAND echo "extern int ${rel_escaped}_start;" >> ${CMAKE_CURRENT_SOURCE_DIR}/externs.hpp
      COMMAND echo "extern int ${rel_escaped}_size;" >> ${CMAKE_CURRENT_SOURCE_DIR}/externs.hpp
      COMMAND ${NASM} ${NASMFLAGS} ${out_f}.asm -o ${out_f}.o
      DEPENDS ${in_f}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Compiling resource file ${out_f}"
      VERBATIM
      )
    list(APPEND result ${out_f})
    list(APPEND objs ${out_f}.o)
  endforeach()
  set(${out_var} "${result}" PARENT_SCOPE)
  set(${objs_var} "${objs}" PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE files ${CMAKE_CURRENT_SOURCE_DIR}/templates/ *.html *.png *.css *.js)
preprocess_resources(files rsrc_objs ${files})

add_library(resources
resource.cpp
resource.hpp
rsrc.cpp
rsrc.hpp
${rsrc_objs})

file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/externs.hpp EXTERNSFILE)
file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/rsrc.hpp RSRCFILE)

set_target_properties(resources PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(resources
PUBLIC
$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

ENDIF()
