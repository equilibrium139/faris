add_executable(magicgen magicgen.cpp)
target_include_directories(magicgen PRIVATE ${CMAKE_SOURCE_DIR})

set(GENERATED_HEADER "${CMAKE_SOURCE_DIR}/magic.h")

# add_custom_command(
#    OUTPUT ${GENERATED_HEADER}
#    COMMAND magicgen ${GENERATED_HEADER}
#    DEPENDS magicgen
#    COMMENT "Generating magic bitboards -> ${GENERATED_HEADER}"
#    VERBATIM
#)

add_custom_target(generate_magic ALL DEPENDS ${GENERATED_HEADER})
