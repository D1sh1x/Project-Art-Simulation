# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\ProjectileTrajectory_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\ProjectileTrajectory_autogen.dir\\ParseCache.txt"
  "ProjectileTrajectory_autogen"
  )
endif()
