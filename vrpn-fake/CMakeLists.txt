if(VRPN_FOUND)
    add_executable(fake fake.cpp)
    target_link_libraries(fake kuhl ${VRPN_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    add_dependencies(fake kuhl)
endif()
