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

#include "CameraFrameTransformer.h"
#include "CameraFrameTransformer_kernel.h"
#include "CameraEngine.h"

const int format_pixel_size[] = { /*UKNOWN*/ -1,/*GRAY*/ 1,/*GRAY16*/ 2, /*RGB*/ 3, /*RGB16*/ 2, /*GRAY16S*/ 2, /*RGB16S*/ 2, /*RAW8*/ 1, /*RAW16*/ 2, /*RGBA*/ 4,
/*YUYV*/ 4, /*UYVY*/2, /*YUV411*/-1, /*YUV444*/-1, /*420P*/-1, /*410P*/-1,/*YVYU*/-1, /*YUV211*/ -1, /*BAYERGRBG*/1,  -1 ,/*JPEG*/-1,/*MJPEG*/-1,/*MJPEG2*/-1,/*MJPEG4*/-1,
/*H263*/-1,/*H264*/-1,/*DVPAL*/-1,-1,-1,-1,/*DVNTSC*/-1 };

CameraFrameTransformer::CameraFrameTransformer()
{
    _dmap = NULL;
    _useJPEGDecompressor = false;
    _jpegBuf = NULL;
}

CameraFrameTransformer::~CameraFrameTransformer()
{
    if(_dmap != NULL) delete[] _dmap;
    
    if(_useJPEGDecompressor)
    {
        tjDestroy(_jpegDecompressor);
        delete[] _jpegBuf;
    }
}

bool CameraFrameTransformer::Init(int src_width, int src_height, int src_format, int dst_width, int dst_height, int dst_format, int dst_xoff, int dst_yoff, bool dst_flip_h, bool dst_flip_v, bool correct_distortion, const char* calib_grid_file)
{
    assert(src_width > 0 && src_height > 0);
    assert(dst_width > 0 && dst_height > 0);
    assert(format_pixel_size[dst_format] > 0);
    
    if(_useJPEGDecompressor)
    {
        tjDestroy(_jpegDecompressor);
        delete[] _jpegBuf;
    }
    
    _useJPEGDecompressor = src_format == FORMAT_JPEG || src_format == FORMAT_MJPEG;
    if(_useJPEGDecompressor)
    {
        assert(dst_format == FORMAT_GRAY || dst_format == FORMAT_RGB);
        
        _jpegDecompressor = tjInitDecompress();
        _jpegBuf = new unsigned char[src_width * dst_width * format_pixel_size[dst_format]];
        
        if(dst_format == FORMAT_GRAY) _jpegPixelFormat = TJPF_GRAY;
        else if(dst_format == FORMAT_RGB) _jpegPixelFormat = TJPF_RGB;

        //Deactivating pixel format conversation in OpenCL
        src_format = dst_format;    
    }
    
    assert(format_pixel_size[src_format] > 0);

    std::vector<cl::Platform> platforms;
    std::vector<cl::Device> devices;

    cl::Platform::get(&platforms);
    if(platforms.size() <= 0)
    {
        std::cout << "error: no OpenCL platform found" << std::endl;
        return false;
    }
    
    platforms[0].getDevices(CL_DEVICE_TYPE_DEFAULT, &devices);
    if(devices.size() <= 0)
    {
        std::cout << "error: no OpenCL device found" << std::endl;
        return false;
    }

    _device = devices[0];

    _context = cl::Context(_device);

    cl::Program::Sources source(1, std::make_pair(kernel_source, kernel_source_size));
    _program = cl::Program(_context, source);

    if(correct_distortion)
    {
        if(calib_grid_file == NULL)
            calib_grid_file = "calibration.grid";

        //TODO: Test on mac
        char config_path[512];
#ifdef __APPLE__
        char path[1024];
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
        CFStringRef cfStringRef = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);
        CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);
        CFRelease(mainBundleURL);
        CFRelease(cfStringRef);
        sprintf(config_path, "%s/Contents/Resources/%s", path, calib_grid_file);
#else
        sprintf(config_path, "./%s", calib_grid_file);
#endif

        this->ComputeDmap(dst_width, dst_height, src_width, src_height, config_path);
    }
    else if(_dmap != NULL)
    {
        delete[] _dmap;
        _dmap = NULL;
    }

    char options[512];
    BuildKernelOptions(options, src_width, src_height, src_format, dst_width, dst_height, dst_format, dst_xoff, dst_yoff, dst_flip_h, dst_flip_v, _dmap != NULL);

    if(_program.build(devices, options) != CL_SUCCESS)
        std::cout << "OpenCL error build kernel: " << _program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(_device) << std::endl;

    _workItemRange = cl::NDRange(dst_height);

    _kernel = cl::Kernel(_program, "transform");
    _queue = cl::CommandQueue(_context, _device);

    _bufferSrcSize = src_width * src_height * format_pixel_size[src_format];
    _bufferSrc = cl::Buffer(_context, CL_MEM_READ_ONLY, _bufferSrcSize);
    _kernel.setArg(0, _bufferSrc);

    _bufferDstSize = dst_width * dst_height * format_pixel_size[dst_format];
    _bufferDst = cl::Buffer(_context, CL_MEM_READ_WRITE, _bufferDstSize);
    _kernel.setArg(1, _bufferDst);

    if(_dmap != NULL)
    {
        _bufferDmapSize = dst_width * dst_height * sizeof(short);
        _bufferDmap = cl::Buffer(_context, CL_MEM_READ_ONLY, _bufferDmapSize);
        _kernel.setArg(2, _bufferDmap);

        //dmap doesn't change till the next invoke of init method, so we only need to write it once
        _queue.enqueueWriteBuffer(_bufferDmap, CL_TRUE, 0, _bufferDmapSize, _dmap);
    }
    else
        _kernel.setArg(2, NULL);
    
    return true;
}

void CameraFrameTransformer::Transform(unsigned char* src, unsigned char* dst)
{    
    if(_useJPEGDecompressor)
    {
        int jpegSubsamp;
        int width, height = 0;
        tjDecompressHeader2(_jpegDecompressor, src, _bufferSrcSize, &width, &height, &jpegSubsamp);
        tjDecompress2(_jpegDecompressor, src, _bufferSrcSize, _jpegBuf, width, 0, height, _jpegPixelFormat, TJFLAG_FASTDCT);
        src = _jpegBuf;
    }
    
    _queue.enqueueWriteBuffer(_bufferSrc, CL_TRUE, 0, _bufferSrcSize, src);
    _queue.enqueueNDRangeKernel(_kernel, cl::NullRange, _workItemRange);
    _queue.enqueueReadBuffer(_bufferDst, CL_TRUE, 0, _bufferDstSize, dst);
    _queue.finish();
}

void CameraFrameTransformer::ComputeDmap(int dst_width, int dst_height, int src_width, int src_height, const char* calib_grid_path)
{
    //_dmap is the distortionmap which tells us where to read a pixel instead of the original position.
    //The size of _dmap depends on the destination image the offsets are based on the source image.
    //With using _dmap on read or rather "before crop" we can move pixels outside of the destination frame into the destination frame

    if(_dmap != NULL)
    {
        delete[] _dmap;
        _dmap = NULL;
    }

    // load the distortion grid
    int grid_size_x = 7;
    if(((float)dst_width / dst_height) > 1.3) grid_size_x += 2;
    if(((float)dst_width / dst_height) > 1.7) grid_size_x += 2;
    int grid_size_y = 7;

    int cell_width = dst_width / (grid_size_x - 1);
    int cell_height = dst_height / (grid_size_y - 1);

    CalibrationGrid grid(grid_size_x, grid_size_y);
    grid.Load(calib_grid_path);

    // we do not calculate the matrix if the grid is not configured
    if(grid.IsEmpty())
        return;

    _dmap = new short[dst_width * dst_height];

    // reset the distortion matrix
    memset(_dmap, 0, dst_width * dst_height * sizeof(short));

    // calculate the distortion matrix
    for(float y = 0; y < dst_height; y++) {
        for(float x = 0; x < dst_width; x++) {

            // get the displacement
            GridPoint new_point = grid.GetInterpolated(x / cell_width, y / cell_height);

            // apply the displacement
            short dx = (short)floor(0.5f + new_point.x*cell_width);
            short dy = (short)floor(0.5f + new_point.y*cell_height);

            int orig_pixel = y*dst_width + x;
            if((x + dx >= 0) && (x + dx < src_width) && (y + dy >= 0) && (y + dy < src_height)) {
                short new_pixel = dy*src_width + dx;
                _dmap[orig_pixel] = new_pixel;
            }
        }
    }
}

void CameraFrameTransformer::BuildKernelOptions(char* options, int src_width, int src_height, int src_format, int dst_width, int dst_height, int dst_format, int dst_xoff, int dst_yoff, bool dst_flip_h, bool dst_flip_v, bool use_dmap)
{
    sprintf(options, "-DSRC_FORMAT=%i -DSRC_FORMAT_PIXEL_SIZE=%i -DSRC_WIDTH=%i -DSRC_HEIGHT=%i -DDST_WIDTH=%i -DDST_HEIGHT=%i -DDST_FORMAT=%i -DDST_FORMAT_PIXEL_SIZE=%i -DDST_XOFF=%i -DDST_YOFF=%i", src_format, format_pixel_size[src_format], src_width, src_height, dst_width, dst_height, dst_format, format_pixel_size[dst_format], dst_xoff, dst_yoff);
    if(dst_flip_h) strcat(options, " -DDST_FLIP_H");
    if(dst_flip_v) strcat(options, " -DDST_FLIP_V");
    if(use_dmap) strcat(options, " -DDMAP");
}