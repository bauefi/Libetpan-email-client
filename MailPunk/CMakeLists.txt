cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

add_executable(MailPunk MailPunk.cpp imap.cpp UI.cpp)
set_property(TARGET MailPunk PROPERTY CXX_STANDARD 17)
target_include_directories(MailPunk SYSTEM PUBLIC ${Project_BINARY_DIR}/deps/include)

# target_link_libraries(MailPunk ${Project_BINARY_DIR}/libetpan-prefix/src/libetpan/src/.libs/libetpan${CMAKE_SHARED_LIBRARY_SUFFIX})

## this is here to work around cmake bugs
ExternalProject_Add(buildfinalcut
  URL "https://github.com/gansm/finalcut/archive/0.5.0.tar.gz"
  PATCH_COMMAND     "./autogen.sh"
  CONFIGURE_COMMAND "./configure" --prefix ${Project_BINARY_DIR}/deps
  BUILD_COMMAND     "make"
  BUILD_IN_SOURCE   TRUE
  BUILD_ALWAYS      FALSE
  )

ExternalProject_Add(buildetpan
	URL "https://github.com/dinhviethoa/libetpan/archive/1.9.1.tar.gz"
	PATCH_COMMAND     "./autogen.sh"
  CONFIGURE_COMMAND "./configure" --prefix ${Project_BINARY_DIR}/deps
  BUILD_COMMAND     "make"
	BUILD_IN_SOURCE   TRUE
	BUILD_ALWAYS      FALSE
  )

ExternalProject_Add(buildfinal
	URL "https://github.com/gansm/finalcut/archive/0.5.0.tar.gz"
	PATCH_COMMAND     "./autogen.sh"
  CONFIGURE_COMMAND "./configure" --prefix ${Project_BINARY_DIR}/deps
  BUILD_COMMAND     "make"
	BUILD_IN_SOURCE   TRUE
	BUILD_ALWAYS      FALSE
  )



link_directories(${Project_BINARY_DIR}/deps/lib)

target_link_libraries(MailPunk ${Project_BINARY_DIR}/deps/lib/${CMAKE_SHARED_LIBRARY_PREFIX}etpan${CMAKE_SHARED_LIBRARY_SUFFIX})
target_link_libraries(MailPunk ${Project_BINARY_DIR}/deps/lib/${CMAKE_SHARED_LIBRARY_PREFIX}final${CMAKE_SHARED_LIBRARY_SUFFIX})


