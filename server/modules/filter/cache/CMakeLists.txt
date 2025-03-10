if (JANSSON_FOUND)
  add_library(cache SHARED
    cache.cc
    cacheconfig.cc
    cachefilter.cc
    cachefiltersession.cc
    cache_storage_api.cc
    cachemt.cc
    cachept.cc
    cachesimple.cc
    cachest.cc
    lrustorage.cc
    lrustoragemt.cc
    lrustoragest.cc
    rules.cc
    sessioncache.cc
    storage.cc
    storagefactory.cc
    )
  target_link_libraries(cache maxscale-common ${JANSSON_LIBRARIES} lzma)
  set_target_properties(cache PROPERTIES VERSION "1.0.0" LINK_FLAGS -Wl,-z,defs)
  install_module(cache core)

  add_subdirectory(storage)

  if(BUILD_TESTS)
    add_subdirectory(test)
  endif()
else()
  message(STATUS "No Jansson libraries found, not building cache filter.")
endif()
