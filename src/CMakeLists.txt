#
# This file is subject to the terms and conditions defined in
# file 'LICENSE', which is part of this source code package.
#

add_library(implant-lib)
target_sources(implant-lib PUBLIC implant.cpp results.cpp tasks.cpp)
target_link_libraries(implant-lib PUBLIC ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

boosttest_add_test(implant-test implant_test.cpp)
target_link_libraries(implant-test PUBLIC implant-lib)

add_executable(implant)
target_sources(implant PRIVATE main.cpp)
target_link_libraries(implant PRIVATE implant-lib ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
