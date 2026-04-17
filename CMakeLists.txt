cmake_minimum_required(VERSION 3.10)

# Enable MSVC Hot Reload (Edit and Continue) in Debug builds
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(ReliableUdpChat)

add_executable(client client.cpp "protocol.h")
add_executable(server server.cpp "protocol.h")

# C++20 for both targets
set_property(TARGET client server PROPERTY CXX_STANDARD 20)

# Ws2_32 is the Windows Sockets library; required for any socket API calls
target_link_libraries(client PRIVATE Ws2_32)
target_link_libraries(server PRIVATE Ws2_32)
