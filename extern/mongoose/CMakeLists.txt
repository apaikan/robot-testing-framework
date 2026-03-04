# Robot Testing Framework
#
# Copyright (C) 2015-2019 Istituto Italiano di Tecnologia (IIT)
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


# Mongoose

set(mongoose_SRCS src/mongoose.c)

set(mongoose_HDRS src/mongoose.h)

add_library(RTF_mongoose OBJECT ${mongoose_SRCS}
                                ${mongoose_HDRS})
add_library(RobotTestingFramework::RTF_mongoose ALIAS RTF_mongoose)

target_include_directories(RTF_mongoose PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
if(NOT WIN32)
  target_link_libraries(RTF_mongoose INTERFACE pthread)
endif()
