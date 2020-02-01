SET(EXAMPLE_NAME "VulkanWicked")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES -DGLM_ENABLE_EXPERIMENTAL")

file(GLOB SOURCE *.cpp)
file(GLOB HEADERS *.h *.hpp)
file(GLOB SHADERS "../data/shaders/*.vert" "../data/shaders/*.frag" "../data/shaders/*.comp" "../data/shaders/*.geom" "../data/shaders/*.tesc" "../data/shaders/*.tese")
source_group("Game\\Shaders" FILES ${SHADERS})
source_group("Game\\Headers" FILES ${HEADERS})
source_group("Game\\Source" FILES ${SOURCE})

file(GLOB RENDERER_SOURCE "renderer/*.cpp")
file(GLOB RENDERER_HEADERS "renderer/*.h" "renderer/*.hpp")
source_group("Renderer\\Source" FILES ${RENDERER_SOURCE})
source_group("Renderer\\Headers" FILES ${RENDERER_HEADERS})

set(KTX_SOURCES
    ../${KTX_DIR}/lib/texture.c
    ../${KTX_DIR}/lib/hashlist.c
    ../${KTX_DIR}/lib/checkheader.c
    ../${KTX_DIR}/lib/swap.c
    ../${KTX_DIR}/lib/memstream.c
    ../${KTX_DIR}/lib/filestream.c
)

file(GLOB ADDITIONAL_SOURCES "../external/imgui/*.cpp")

if(WIN32)
	add_executable(${EXAMPLE_NAME} WIN32 main.cpp ${SOURCE} ${HEADERS} ${SHADERS} ${RENDERER_SOURCE} ${RENDERER_HEADERS} ${ADDITIONAL_SOURCES} ${KTX_SOURCES})
	target_link_libraries(${EXAMPLE_NAME} ${SDL2_LIBRARIES} ${Vulkan_LIBRARY} ${WINLIBS})
else(WIN32)
	add_executable(${EXAMPLE_NAME} ${MAIN_CPP} ${SOURCE} ${SHADERS})
	target_link_libraries(${EXAMPLE_NAME})
endif(WIN32)
if(RESOURCE_INSTALL_DIR)
	install(TARGETS ${EXAMPLE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()