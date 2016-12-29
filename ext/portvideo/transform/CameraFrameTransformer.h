/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CAMERAFRAMETRANSFORMER_H
#define CAMERAFRAMETRANSFORMER_H

//deaktivating OpenCL1.2 was necessary to build the project
#include <CL/cl.h>
#undef CL_VERSION_1_2
#include <CL/cl.hpp>
#include <vector>
#include <iostream>
#include <assert.h>
#include <CalibrationGrid.h>
#include "floatpoint.h"

class CameraFrameTransformer
{
public:
    CameraFrameTransformer();
    ~CameraFrameTransformer();
    void Init(int src_width, int src_height, int src_format, int dst_width, int dst_height, int dst_format, int dst_xoff, int dst_yoff, bool dst_flip_h, bool dst_flip_v, bool correct_distortion = true, char* calib_grid_file = NULL);
    void Transform(unsigned char* src, unsigned char* dst);

private:
    cl::Context _context;
    cl::Program _program;
    cl::Device _device;
    cl::CommandQueue _queue;
    cl::NDRange _workItemRange;

    cl::Kernel _kernel;
    cl::Buffer _bufferSrc;
    cl::Buffer _bufferDst;
    cl::Buffer _bufferDmap;

    int _bufferSrcSize;
    int _bufferDstSize;
    int _bufferDmapSize;

    short* _Dmap;

    void ComputeDmap(int dst_width, int dst_height, int src_width, int src_height, char* calib_grid_path);
    static void BuildKernelOptions(char* options, int src_width, int src_height, int src_format, int dst_width, int dst_height, int dst_format, int dst_xoff, int dst_yoff, bool dst_flip_h, bool dst_flip_v, bool use_dmap);
};

#endif
