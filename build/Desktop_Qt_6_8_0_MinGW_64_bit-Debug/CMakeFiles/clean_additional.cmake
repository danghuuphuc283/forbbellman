# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\fordbellman_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\fordbellman_autogen.dir\\ParseCache.txt"
  "fordbellman_autogen"
  )
endif()
