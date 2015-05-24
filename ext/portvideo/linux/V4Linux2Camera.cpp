/* portVideo, a cross platform camera framework
 Copyright (C) 2005-2015 Martin Kaltenbrunner <martin@tuio.org>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "V4Linux2Camera.h"

V4Linux2Camera::V4Linux2Camera(const char* cfg) : CameraEngine(cfg)
{
    cameraID = -1;

    comps = new int;

    cam_buffer = NULL;
    crop_buffer = NULL;
    running = false;
    buffers_initialized = false;

    memset(&v4l2_auto_ctrl,0,sizeof(v4l2_auto_ctrl));
}

V4Linux2Camera::~V4Linux2Camera(void)
{
    if (v4l2_form.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG) tjDestroy(_jpegDecompressor);
    if (cam_buffer) delete cam_buffer;
    if (crop_buffer) delete crop_buffer;
}

bool V4Linux2Camera::initCamera() {

    if (cameraID < 0) return false;
    memset(&v4l2_form, 0, sizeof(v4l2_format));

    // select pixelformat
    for (int i=0;;i++) {
        struct v4l2_fmtdesc fmtdesc;
        memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
        fmtdesc.index = i;
        fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl(cameraID,VIDIOC_ENUM_FMT,&fmtdesc)) break;

        if ((config.compress) && (fmtdesc.pixelformat==V4L2_PIX_FMT_MJPEG)) {
            v4l2_form.fmt.pix.pixelformat = fmtdesc.pixelformat;
            break;
        } else if (!(config.compress) && ((fmtdesc.pixelformat==V4L2_PIX_FMT_YUYV) || (fmtdesc.pixelformat==V4L2_PIX_FMT_YUYV) || (fmtdesc.pixelformat==V4L2_PIX_FMT_UYVY) || (fmtdesc.pixelformat==V4L2_PIX_FMT_YUV420) || (fmtdesc.pixelformat==V4L2_PIX_FMT_YUV410 || (fmtdesc.pixelformat==V4L2_PIX_FMT_GREY)))) {
            v4l2_form.fmt.pix.pixelformat = fmtdesc.pixelformat;
            break;
        }
    }

    cam_width = 0;
    cam_height = 0;
    unsigned int max_width = 0;
    unsigned int max_height = 0;
    unsigned int min_width = INT_MAX;
    unsigned int min_height = INT_MAX;

    // select size
    for (int i=0;;i++) {
        struct v4l2_frmsizeenum frmsize;
        memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
        frmsize.index = i;
        frmsize.pixel_format  = v4l2_form.fmt.pix.pixelformat;
        if (-1 == ioctl(cameraID,VIDIOC_ENUM_FRAMESIZES,&frmsize)) break;

        if (frmsize.discrete.width>max_width) max_width=frmsize.discrete.width;
        if (frmsize.discrete.height>max_height) max_height=frmsize.discrete.height;
        if (frmsize.discrete.width<min_width) min_width=frmsize.discrete.width;
        if (frmsize.discrete.height<min_height) min_height=frmsize.discrete.height;

        if ((config.cam_width==(int)frmsize.discrete.width) && (config.cam_height==(int)frmsize.discrete.height)) {
            cam_width = frmsize.discrete.width;
            cam_height = frmsize.discrete.height;
            break;
        }
    }

    if (config.cam_width==SETTING_MAX) cam_width = max_width;
    if (config.cam_height==SETTING_MAX) cam_height = max_height;
    if (config.cam_width==SETTING_MIN) cam_width = min_width;
    if (config.cam_height==SETTING_MIN) cam_height = min_height;
    if (cam_width==0) cam_width = min_width;
    if (cam_height==0) cam_height = min_height;

    fps = 0;
    float max_fps = 0;
    float min_fps = INT_MAX;

    // select fps
    float last_fps = 0.0f;
    for (int i=0;;i++) {
        struct v4l2_frmivalenum frmival;
        memset(&frmival, 0, sizeof(v4l2_frmivalenum));
        frmival.index = i;
        frmival.pixel_format = v4l2_form.fmt.pix.pixelformat;
        frmival.width = cam_width;
        frmival.height = cam_height;
        if (-1 == ioctl(cameraID,VIDIOC_ENUM_FRAMEINTERVALS,&frmival)) break;

        float frm_fps = frmival.discrete.denominator/(float)frmival.discrete.numerator;
        if(frm_fps==last_fps) break;
        last_fps=frm_fps;

        if(frm_fps>max_fps) max_fps = frm_fps;
        if(frm_fps<min_fps) min_fps = frm_fps;
    }

    if (config.cam_fps==SETTING_MAX) fps = max_fps;
    if (config.cam_fps==SETTING_MIN) fps = min_fps;
    if (config.cam_fps>max_fps) fps = max_fps;
    if (config.cam_fps<min_fps) fps = min_fps;
    if (fps==0) fps = config.cam_fps;

    // try to set the desired dimensions
    v4l2_form.fmt.pix.width  = cam_width;
    v4l2_form.fmt.pix.height = cam_height;
    v4l2_parm.parm.capture.timeperframe.numerator = 1;
    v4l2_parm.parm.capture.timeperframe.denominator = int(fps);

    v4l2_form.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl (cameraID, VIDIOC_S_FMT, &v4l2_form)) {
        printf("error setting pixel format: %s\n" , strerror(errno));
        return false;
    }

    // use the settings we got from the driver
    pixelformat = v4l2_form.fmt.pix.pixelformat;
    cam_width = v4l2_form.fmt.pix.width;
    cam_height = v4l2_form.fmt.pix.height;
    fps = v4l2_parm.parm.capture.timeperframe.denominator;

    if (v4l2_form.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG) _jpegDecompressor = tjInitDecompress();

    if (!requestBuffers()) {
        printf("Error requesting buffers.\n");
        return false;
    }

    if (!mapBuffers()) {
        printf("Unable to mmap buffers.\n");
        return false;
    }

    setupFrame();
    if (config.frame) crop_buffer = new unsigned char[frame_width*frame_height*bytes];
    cam_buffer = new unsigned char[cam_width*cam_height*bytes];
    buffers_initialized = true;
    return true;
}

int v4lfilter(const struct dirent *dir)
{
    const char *name = dir->d_name;
    int len = strlen(name);
    if(len >= 0) {
        if (strncmp(name, "video", 5) == 0) return 1;
    }

    return 0;
}

void V4Linux2Camera::listDevices() {

    char v4l2_device[128];
    v4l2_capability v4l2_caps;
    struct dirent **v4l2_devices;

    int device_count = scandir ("/dev/", &v4l2_devices, v4lfilter, alphasort);
    if (device_count==0) {
        printf("no V4Linux2 devices found\n");
        return;
    }

    if (device_count==1) printf("1 V4Linux2 device found:\n");
    else printf("%d V4Linux2 devices found:\n",device_count);

    for (int i=0;i<device_count;i++) {
        sprintf(v4l2_device,"/dev/%s",v4l2_devices[i]->d_name);

        int fd = open(v4l2_device, O_RDONLY);
        if (fd < 0) continue;

        memset(&v4l2_caps, 0, sizeof(v4l2_capability));
        if (ioctl(fd, VIDIOC_QUERYCAP, &v4l2_caps) < 0) {
            printf("%s oops\n",v4l2_device);
            close(fd);
            continue;
        }

        if (v4l2_caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
            printf("\t%s: %s (%s)\n", v4l2_device, v4l2_caps.card, v4l2_caps.driver);

            for (int x=0;;x++) {
                struct v4l2_fmtdesc fmtdesc;
                memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
                fmtdesc.index = x;
                fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)) break;
                printf("\t\tformat: %s%s\n",  fmtdesc.description, fmtdesc.flags & V4L2_FMT_FLAG_COMPRESSED ? " (compressed)" : "");

                for (int y=0;;y++) {
                    struct v4l2_frmsizeenum frmsize;
                    memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
                    frmsize.index = y;
                    frmsize.pixel_format  = fmtdesc.pixelformat;
                    if (-1 == ioctl(fd,VIDIOC_ENUM_FRAMESIZES,&frmsize)) break;
                    printf("\t\t\t %dx%d ",frmsize.discrete.width,frmsize.discrete.height);

                    float last_fps=0.0f;
                    for (int z=0;;z++) {
                        struct v4l2_frmivalenum frmival;
                        memset(&frmival, 0, sizeof(v4l2_frmivalenum));
                        frmival.index = z;
                        frmival.pixel_format = fmtdesc.pixelformat;
                        frmival.width = frmsize.discrete.width;
                        frmival.height = frmsize.discrete.height;
                        if (-1 == ioctl(fd,VIDIOC_ENUM_FRAMEINTERVALS,&frmival)) break;

                        float frm_fps = frmival.discrete.denominator/(float)frmival.discrete.numerator;
                        if(frm_fps==last_fps) break;
                        last_fps=frm_fps;

                        if (int(frm_fps) == frm_fps) printf("%d|",int(frm_fps));
                        else printf("%'.1f|",frm_fps);
                    }  printf("\b fps\n");
                }
            }
        }
        close(fd);
    }
}

bool V4Linux2Camera::findCamera() {

    readSettings();
    if (config.device<0) config.device=0;

    struct dirent **v4l2_devices;
    int device_count = scandir ("/dev/", &v4l2_devices, v4lfilter, alphasort);
    if (device_count==0) {
        printf("no V4Linux2 devices found\n");
        return false;
    } else if (device_count==1) printf("1 V4Linux2 device found\n");
    else printf("%d V4Linux2 devices found\n",device_count);

    char v4l2_device[128];
    sprintf(v4l2_device,"/dev/video%d",config.device);

    int fd = open(v4l2_device, O_RDWR);
    if (fd < 0) {
        printf("no V4Linux2 device found at %s\n", v4l2_device);
        return false;
    }

    memset(&v4l2_caps, 0, sizeof(v4l2_capability));
    if (ioctl(fd, VIDIOC_QUERYCAP, &v4l2_caps) < 0) {
        printf("%s does not support video capture\n",v4l2_device);
        close(fd);
        return false;
    }

    if ((v4l2_caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
        printf("%s does not support video capture\n",v4l2_device);
        close(fd);
        return false;
    }

    if ((v4l2_caps.capabilities & V4L2_CAP_STREAMING) == 0) {
        printf("%s does not support memory streaming capture\n",v4l2_device);
        close(fd);
        return false;
    }

    bool valid_compressed_format = false;
    bool valid_uncompressed_format = false;
    for (int i=0;;i++) {
        struct v4l2_fmtdesc fmtdesc;
        memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
        fmtdesc.index = i;
        fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)) break;

        if ((fmtdesc.flags & V4L2_FMT_FLAG_COMPRESSED) && (fmtdesc.pixelformat==V4L2_PIX_FMT_MJPEG)) {
            valid_compressed_format = true;
            //printf("got compressed format\n");
        } else if (!(fmtdesc.flags & V4L2_FMT_FLAG_COMPRESSED) && ((fmtdesc.pixelformat==V4L2_PIX_FMT_YUYV) || (fmtdesc.pixelformat==V4L2_PIX_FMT_YUYV) || (fmtdesc.pixelformat==V4L2_PIX_FMT_UYVY) || (fmtdesc.pixelformat==V4L2_PIX_FMT_YUV420) || (fmtdesc.pixelformat==V4L2_PIX_FMT_YUV410 || (fmtdesc.pixelformat==V4L2_PIX_FMT_GREY)))) {
            valid_uncompressed_format = true;
            //printf("got uncompressed format\n");
        }
    }

    if ((valid_uncompressed_format==false) && (valid_compressed_format==false)) {
        printf("%s does not support any valid pixel format\n",v4l2_device);
        close(fd);
        return false;
    }

    if ((config.compress==true) && (valid_compressed_format==false)) config.compress = false;
    if ((config.compress==false) && (valid_uncompressed_format==false)) config.compress = true;

    sprintf(cameraName, "%s (%s)", v4l2_caps.card, v4l2_caps.driver);
    cameraID = fd;
    return true;
}

bool V4Linux2Camera::startCamera() {

    for(unsigned int i = 0; i < v4l2_reqbuffers.count; ++i) {
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        v4l2_buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.memory      = V4L2_MEMORY_MMAP;
        v4l2_buf.index       = i;

        if (-1 == ioctl (cameraID, VIDIOC_QBUF, &v4l2_buf)) {
            printf("Error queuing buffer: %s\n",  strerror(errno));
            exit(0);
        }
    }

    enum v4l2_buf_type type;
    memset(&type, 0, sizeof(v4l2_buf_type));
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cameraID, VIDIOC_STREAMON, &type) < 0) {
        printf("Cannot start camera: %s\n", strerror(errno));
        return false;
    }

    applyCameraSettings();
    running = true;
    return true;
}

unsigned char* V4Linux2Camera::getFrame()  {

    if (cameraID==-1) return NULL;

    if (ioctl(cameraID, VIDIOC_DQBUF, &v4l2_buf)<0) {
        running = false;
        return NULL;
    }

    unsigned char *raw_buffer = (unsigned char*)buffers[v4l2_buf.index].start;
    if (raw_buffer==NULL) return NULL;

    if(colour) {
        if (config.frame) {
         if (pixelformat==V4L2_PIX_FMT_YUYV)
            crop_yuyv2rgb(cam_width,raw_buffer,crop_buffer);
         else if (pixelformat==V4L2_PIX_FMT_UYVY)
            crop_uyvy2rgb(cam_width,raw_buffer,crop_buffer);
         else if (pixelformat==V4L2_PIX_FMT_YUV420) { //TODO
         } else if (pixelformat==V4L2_PIX_FMT_YUV410) { //TODO
         } else if (pixelformat==V4L2_PIX_FMT_GREY)
            crop_gray2rgb(cam_width,raw_buffer, crop_buffer);
         else if (pixelformat==V4L2_PIX_FMT_MJPEG) {
                int jpegSubsamp;
                tjDecompressHeader2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, &cam_width, &cam_height, &jpegSubsamp);
                tjDecompress2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, cam_buffer, cam_width, 0, cam_height, TJPF_RGB, TJFLAG_FASTDCT);
                cropFrame(cam_buffer,crop_buffer,3);
         }

        } else {
         if (pixelformat==V4L2_PIX_FMT_YUYV)
            yuyv2rgb(cam_width,cam_height,raw_buffer,cam_buffer);
         else if (pixelformat==V4L2_PIX_FMT_UYVY)
            uyvy2rgb(cam_width,cam_height,raw_buffer,cam_buffer);
         else if (pixelformat==V4L2_PIX_FMT_YUV420) { //TODO
         } else if (pixelformat==V4L2_PIX_FMT_YUV410) { //TODO
         } else if (pixelformat==V4L2_PIX_FMT_GREY)
            gray2rgb(cam_width,cam_height,raw_buffer,cam_buffer);
         else if (pixelformat==V4L2_PIX_FMT_MJPEG) {
                int jpegSubsamp;
                tjDecompressHeader2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, &cam_width, &cam_height, &jpegSubsamp);
         }
                tjDecompress2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, cam_buffer, cam_width, 0, cam_height, TJPF_RGB, TJFLAG_FASTDCT);

        }

    } else {
        if (config.frame) {
            if (pixelformat==V4L2_PIX_FMT_YUYV)
                crop_yuyv2gray(cam_width,raw_buffer,crop_buffer);
            else if (pixelformat==V4L2_PIX_FMT_UYVY)
                crop_uyvy2gray(cam_width,raw_buffer,crop_buffer);
            else if (pixelformat==V4L2_PIX_FMT_YUV420)
                cropFrame(raw_buffer,crop_buffer,1);
            else if (pixelformat==V4L2_PIX_FMT_YUV410)
                cropFrame(raw_buffer,crop_buffer,1);
            else if (pixelformat==V4L2_PIX_FMT_GREY)
                cropFrame(raw_buffer,crop_buffer,1);
            else if (pixelformat==V4L2_PIX_FMT_MJPEG) {

                int jpegSubsamp;
                tjDecompressHeader2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, &cam_width, &cam_height, &jpegSubsamp);
                tjDecompress2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, cam_buffer, cam_width, 0, cam_height, TJPF_GRAY, TJFLAG_FASTDCT);
                cropFrame(cam_buffer,crop_buffer,1);
            }
        } else {
            if (pixelformat==V4L2_PIX_FMT_YUYV) yuyv2gray(cam_width, cam_height, raw_buffer, cam_buffer);
            else if (pixelformat==V4L2_PIX_FMT_UYVY) uyvy2gray(cam_width, cam_height, raw_buffer, cam_buffer);
            else if (pixelformat==V4L2_PIX_FMT_YUV420) memcpy(cam_buffer,raw_buffer,cam_width*cam_height);
            else if (pixelformat==V4L2_PIX_FMT_YUV410) memcpy(cam_buffer,raw_buffer,cam_width*cam_height);
            //else if (pixelformat==V4L2_PIX_FMT_GREY) memcpy(cam_buffer,raw_buffer,cam_width*cam_height);
            else if (pixelformat==V4L2_PIX_FMT_MJPEG)  {

                int jpegSubsamp;
                tjDecompressHeader2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, &cam_width, &cam_height, &jpegSubsamp);
                tjDecompress2(_jpegDecompressor, raw_buffer, v4l2_buf.bytesused, cam_buffer, cam_width, 0, cam_height, TJPF_GRAY, TJFLAG_FASTDCT);
            }
        }
    }

    if (-1 == ioctl (cameraID, VIDIOC_QBUF, &v4l2_buf)) {
        printf("cannot unqueue buffer: %s\n", strerror(errno));
        return NULL;
    }

    if (config.frame) return crop_buffer;
    else if ((!colour) && (pixelformat==V4L2_PIX_FMT_GREY)) return raw_buffer;
    else return cam_buffer;
}

bool V4Linux2Camera::stopCamera() {
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cameraID, VIDIOC_STREAMOFF, &type) < 0) {
        //printf("cannot stop camera: %s\n", strerror(errno));
        return false;
    }

    running = false;
    return true;
}

bool V4Linux2Camera::stillRunning() {
 	  return running;
}

bool V4Linux2Camera::resetCamera()
{
    return (stopCamera() && startCamera());
}


bool V4Linux2Camera::closeCamera() {

    if (cameraID >= 0) {
        if (buffers_initialized) unmapBuffers();
        close(cameraID);
        cameraID = -1;

        return true;
    } else return false;

}

bool V4Linux2Camera::requestBuffers() {
    // Request mmap buffers
    memset (&v4l2_reqbuffers, 0, sizeof (v4l2_reqbuffers));
    v4l2_reqbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_reqbuffers.memory = V4L2_MEMORY_MMAP;
    v4l2_reqbuffers.count = nr_of_buffers;

    if (-1 == ioctl (cameraID, VIDIOC_REQBUFS, &v4l2_reqbuffers)) {
        if (errno == EINVAL)
            printf ("video capturing or mmap-streaming is not supported\n");
        else
            perror ("VIDIOC_REQBUFS");

        return false;
    }

    if (v4l2_reqbuffers.count < 1) {
        /* You may need to free the buffers here. */
        printf("Error requesting buffers.\n");
        return false;
    }

    return true;
}

bool V4Linux2Camera::mapBuffers() {

    for(unsigned int i=0; i<v4l2_reqbuffers.count; i++) {

        memset (&v4l2_buf, 0, sizeof (v4l2_buf));
        v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        v4l2_buf.index = i;

        if (-1 == ioctl (cameraID, VIDIOC_QUERYBUF, &v4l2_buf)) {
            printf("Unable to query buffer: %s\n",  strerror(errno));
            return false;
        }

        buffers[i].length = v4l2_buf.length; /* remember for munmap() */
        buffers[i].start = mmap (NULL, v4l2_buf.length,
                                 PROT_READ | PROT_WRITE, /* recommended */
                                 MAP_SHARED,             /* recommended */
                                 cameraID, v4l2_buf.m.offset);

        if (MAP_FAILED == buffers[i].start) {
            /* If you do not exit here you should unmap() and free()
             the buffers mapped so far. */
            printf("Unable to map buffer %d.\n",  i);
            return false;
        }
    }

    return true;
}

bool V4Linux2Camera::unmapBuffers() {
    for(unsigned int i=0; i<v4l2_reqbuffers.count; i++) {
        if (-1 == munmap(buffers[i].start, buffers[i].length)) {
            printf("Error unmapping buffer %d.\n", i);
            return false;
        }
    }
    return true;
}

bool V4Linux2Camera::hasCameraSettingAuto(int mode) {

    v4l2_queryctrl v4l2_query;

    switch (mode) {
        case GAIN: v4l2_query.id = V4L2_CID_AUTOGAIN; break;
        case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_AUTO; break;
        case WHITE: v4l2_query.id = V4L2_CID_AUTO_WHITE_BALANCE; break;
        case FOCUS: v4l2_query.id = V4L2_CID_FOCUS_AUTO; break;
        case COLOR_HUE: v4l2_query.id = V4L2_CID_HUE_AUTO; break;
        default: return false;
    }

    if (( ioctl(cameraID, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
        return false;
    } else if (v4l2_query.flags & V4L2_CTRL_FLAG_DISABLED) {
        return false;
    } else return true;

}

static int xioctl(int fd, int request, void * arg)
{
	int r;

	do
	r = ioctl(fd, request, arg); while (-1==r&&EINTR==errno);

	return r;
}

bool V4Linux2Camera::getCameraSettingAuto(int mode) {

    if (!hasCameraSetting(mode)) return false;
    //v4l2_ext_control v4l2_ctrl[2];
    //v4l2_control v4l2_ctrl;
    //memset(&v4l2_ctrl, 0, sizeof (v4l2_ctrl));

    switch (mode) {
        case GAIN: return getCameraSetting(AUTO_GAIN);
        case EXPOSURE:
            v4l2_auto_ctrl[0].id = V4L2_CID_EXPOSURE_AUTO;
            v4l2_auto_ctrl[1].id = V4L2_CID_EXPOSURE_AUTO_PRIORITY;
           break;
        case WHITE: return getCameraSetting(AUTO_WHITE);
        case COLOR_HUE: return getCameraSetting(AUTO_HUE);
        default: return false;
    }

    if ((xioctl(cameraID, VIDIOC_G_EXT_CTRLS, &v4l2_auto_ctrl)) < 0) {
        printf("Unable to get AUTO mode: %s\n" , strerror(errno));
        return false;
    }

    if (mode==EXPOSURE) {
        printf("get auto mode: %d %d\n",v4l2_auto_ctrl[0].value,v4l2_auto_ctrl[1].value);
        return (v4l2_auto_ctrl[1].value<1);
    } else return false;
}

bool V4Linux2Camera::setCameraSettingAuto(int mode, bool flag) {

    if (!hasCameraSetting(mode)) return false;

    switch (mode) {
        case GAIN:
           return(setCameraSetting(AUTO_GAIN,flag));
        case EXPOSURE:
            v4l2_auto_ctrl[0].id = V4L2_CID_EXPOSURE_AUTO;
            v4l2_auto_ctrl[1].id = V4L2_CID_EXPOSURE_AUTO_PRIORITY;
            if (flag==true) {
                v4l2_auto_ctrl[0].value = V4L2_EXPOSURE_AUTO;
                v4l2_auto_ctrl[1].value = V4L2_EXPOSURE_AUTO;
            } else {
                v4l2_auto_ctrl[0].value = V4L2_EXPOSURE_MANUAL;
                v4l2_auto_ctrl[1].value = V4L2_EXPOSURE_APERTURE_PRIORITY;
           }
            break;
        case WHITE:
            return(setCameraSetting(AUTO_WHITE,flag));
        case COLOR_HUE:
            return(setCameraSetting(AUTO_HUE,flag));
        default: return false;
    }
    if ((xioctl(cameraID, VIDIOC_S_EXT_CTRLS, &v4l2_auto_ctrl)) < 0) {
        sprintf("Unable to set AUTO mode: %s\n" , strerror(errno));
        return false;
    }

    if (mode==EXPOSURE) {
        printf("set auto mode: %d %d\n",v4l2_auto_ctrl[0].value,v4l2_auto_ctrl[1].value);
        return true;
    }

    return false;
}

bool V4Linux2Camera::hasCameraSetting(int mode) {

    struct v4l2_queryctrl v4l2_query = {0};

    switch (mode) {
        case BRIGHTNESS: v4l2_query.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_query.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_query.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_query.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN: return hasCameraSettingAuto(GAIN);
        case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: return hasCameraSettingAuto(EXPOSURE);
        case WHITE: v4l2_query.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: return hasCameraSettingAuto(WHITE);
        case FOCUS: v4l2_query.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: return hasCameraSettingAuto(FOCUS);
        case BACKLIGHT: v4l2_query.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case POWERLINE: v4l2_query.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case GAMMA: v4l2_query.id = V4L2_CID_GAMMA; break;
        case COLOR_HUE: v4l2_query.id = V4L2_CID_HUE; break;
        case AUTO_HUE: return hasCameraSettingAuto(COLOR_HUE);
        case COLOR_RED: v4l2_query.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_query.id = V4L2_CID_BLUE_BALANCE; break;
        default: return false;
    }

    if (( ioctl(cameraID, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
        return false;
    } else if (v4l2_query.flags & V4L2_CTRL_FLAG_DISABLED) {
        return false;
    } return true;
}

bool V4Linux2Camera::setCameraSetting(int mode, int setting) {

    if (!hasCameraSetting(mode)) return false;
    struct v4l2_control v4l2_ctrl = {0};
    v4l2_ctrl.value = setting;

    switch (mode) {
        case BRIGHTNESS: v4l2_ctrl.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_ctrl.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_ctrl.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_ctrl.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN:  v4l2_ctrl.id = V4L2_CID_AUTOGAIN; break;
        case EXPOSURE: v4l2_ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: return setCameraSettingAuto(EXPOSURE,setting);
        case WHITE: v4l2_ctrl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: v4l2_ctrl.id = V4L2_CID_AUTO_WHITE_BALANCE; break;
        case FOCUS: v4l2_ctrl.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: return setCameraSettingAuto(FOCUS,setting);
        case BACKLIGHT: v4l2_ctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case POWERLINE: v4l2_ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case GAMMA: v4l2_ctrl.id = V4L2_CID_GAMMA; break;
        case COLOR_HUE: v4l2_ctrl.id = V4L2_CID_HUE; break;
        case AUTO_HUE: v4l2_ctrl.id = V4L2_CID_HUE_AUTO; break;
        case COLOR_RED: v4l2_ctrl.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_ctrl.id = V4L2_CID_BLUE_BALANCE; break;
        default: return false;
    }

    if ((ioctl(cameraID, VIDIOC_S_CTRL, &v4l2_ctrl)) < 0) {
        //printf("Unable to set value: %s\n" , strerror(errno));
        return false;
    } return true;
}

int V4Linux2Camera::getCameraSetting(int mode) {

    if (!hasCameraSetting(mode)) return 0;
    v4l2_control v4l2_ctrl;

    switch (mode) {
        case BRIGHTNESS: v4l2_ctrl.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_ctrl.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_ctrl.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_ctrl.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN: v4l2_ctrl.id = V4L2_CID_AUTOGAIN; break;
        case EXPOSURE: v4l2_ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: return getCameraSettingAuto(EXPOSURE); break;
        case WHITE: v4l2_ctrl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: v4l2_ctrl.id = V4L2_CID_AUTO_WHITE_BALANCE; break;
        case FOCUS: v4l2_ctrl.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: return getCameraSettingAuto(FOCUS);
        case BACKLIGHT: v4l2_ctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case POWERLINE: v4l2_ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case GAMMA: v4l2_ctrl.id = V4L2_CID_GAMMA; break;
        case COLOR_HUE: v4l2_ctrl.id = V4L2_CID_HUE; break;
        case AUTO_HUE: v4l2_ctrl.id = V4L2_CID_HUE_AUTO; break;
        case COLOR_RED: v4l2_ctrl.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_ctrl.id = V4L2_CID_BLUE_BALANCE; break;
        default: return 0;
    }

    if ((ioctl(cameraID, VIDIOC_G_CTRL, &v4l2_ctrl)) < 0) {
        return 0;
    } else return v4l2_ctrl.value;
}

bool V4Linux2Camera::setDefaultCameraSetting(int mode) {
    return setCameraSetting(mode,getDefaultCameraSetting(mode));
}

int V4Linux2Camera::getDefaultCameraSetting(int mode) {

    if (!hasCameraSetting(mode)) return 0;
    v4l2_queryctrl v4l2_query;

    switch (mode) {
        case BRIGHTNESS: v4l2_query.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_query.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_query.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_query.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN: v4l2_query.id = V4L2_CID_AUTOGAIN; break;
        case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_AUTO; break;
        case FOCUS: v4l2_query.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: v4l2_query.id = V4L2_CID_FOCUS_AUTO; break;
        case WHITE: v4l2_query.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: v4l2_query.id = V4L2_CID_AUTO_WHITE_BALANCE; break;
        case GAMMA: v4l2_query.id = V4L2_CID_GAMMA; break;
        case BACKLIGHT: v4l2_query.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case POWERLINE: v4l2_query.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case COLOR_HUE: v4l2_query.id = V4L2_CID_HUE; break;
        case AUTO_HUE: v4l2_query.id = V4L2_CID_HUE_AUTO; break;
        case COLOR_RED: v4l2_query.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_query.id = V4L2_CID_BLUE_BALANCE; break;
        default: return 0;
    }

    if (( ioctl(cameraID, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
        return 0;
    } else if (v4l2_query.flags & V4L2_CTRL_FLAG_DISABLED) {
        return 0;
    } else if (v4l2_query.type & V4L2_CTRL_TYPE_INTEGER) {
        return v4l2_query.default_value;
    }

    return 0;
}

int V4Linux2Camera::getMaxCameraSetting(int mode) {

    if (!hasCameraSetting(mode)) return 0;
    v4l2_queryctrl v4l2_query;

    switch (mode) {
        case BRIGHTNESS: v4l2_query.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_query.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_query.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_query.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN: return 1;
        case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: return 1;
        case FOCUS: v4l2_query.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: return 1;
        case WHITE: v4l2_query.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: return 1;
        case GAMMA: v4l2_query.id = V4L2_CID_GAMMA; break;
        case BACKLIGHT: v4l2_query.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case POWERLINE: v4l2_query.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case COLOR_HUE: v4l2_query.id = V4L2_CID_HUE; break;
        case AUTO_HUE: return 1;
        case COLOR_RED: v4l2_query.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_query.id = V4L2_CID_BLUE_BALANCE; break;
        default: return 0;
    }

    if (( ioctl(cameraID, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
        return 0;
    } else if (v4l2_query.flags & V4L2_CTRL_FLAG_DISABLED) {
        return 0;
    } else if (v4l2_query.flags & V4L2_CTRL_TYPE_BOOLEAN) {
        return 1;
    } else if (v4l2_query.type & V4L2_CTRL_TYPE_INTEGER) {
        return v4l2_query.maximum;
    }

    return 0;
}

int V4Linux2Camera::getMinCameraSetting(int mode) {

    if (!hasCameraSetting(mode)) return 0;
    struct v4l2_queryctrl v4l2_query = {0};

    switch (mode) {
        case BRIGHTNESS: v4l2_query.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_query.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_query.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_query.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN: return 0;
        case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: return 0;
        case FOCUS: v4l2_query.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: return 0;
        case WHITE: v4l2_query.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: return 0;
        case GAMMA: v4l2_query.id = V4L2_CID_GAMMA; break;
        case BACKLIGHT: v4l2_query.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case POWERLINE: v4l2_query.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case COLOR_HUE: v4l2_query.id = V4L2_CID_HUE; break;
        case AUTO_HUE: return 0;
        case COLOR_RED: v4l2_query.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_query.id = V4L2_CID_BLUE_BALANCE; break;
        default: return 0;
    }

    if (( ioctl(cameraID, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
    } else if (v4l2_query.flags & V4L2_CTRL_FLAG_DISABLED) {
        return 0;
        return 0;
    } else if (v4l2_query.flags & V4L2_CTRL_TYPE_BOOLEAN) {
        return 1;
    } else if (v4l2_query.type & V4L2_CTRL_TYPE_INTEGER) {
        return v4l2_query.minimum;
    }

    return 0;
}

int V4Linux2Camera::getCameraSettingStep(int mode) {

    struct v4l2_queryctrl v4l2_query = {0};

    switch (mode) {
        case BRIGHTNESS: v4l2_query.id = V4L2_CID_BRIGHTNESS; break;
        case CONTRAST: v4l2_query.id = V4L2_CID_CONTRAST; break;
        case SHARPNESS: v4l2_query.id = V4L2_CID_SHARPNESS; break;
        case GAIN: v4l2_query.id = V4L2_CID_GAIN; break;
        case AUTO_GAIN: return 1;
        case EXPOSURE: v4l2_query.id = V4L2_CID_EXPOSURE_ABSOLUTE; break;
        case AUTO_EXPOSURE: return 1;
        case FOCUS: v4l2_query.id = V4L2_CID_FOCUS_ABSOLUTE; break;
        case AUTO_FOCUS: return 1;
        case WHITE: v4l2_query.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE; break;
        case AUTO_WHITE: return 1;
        case GAMMA: v4l2_query.id = V4L2_CID_GAMMA; break;
        case POWERLINE: v4l2_query.id = V4L2_CID_POWER_LINE_FREQUENCY; break;
        case BACKLIGHT: v4l2_query.id = V4L2_CID_BACKLIGHT_COMPENSATION; break;
        case COLOR_HUE: v4l2_query.id = V4L2_CID_HUE; break;
        case AUTO_HUE: return 1;
        case COLOR_RED: v4l2_query.id = V4L2_CID_RED_BALANCE; break;
        case COLOR_BLUE: v4l2_query.id = V4L2_CID_BLUE_BALANCE; break;
        default: return 0;
    }

    if (( ioctl(cameraID, VIDIOC_QUERYCTRL, &v4l2_query)) < 0) {
        return 0;
    } else if (v4l2_query.flags & V4L2_CTRL_FLAG_DISABLED) {
        return 0;
    } else if (v4l2_query.flags & V4L2_CTRL_TYPE_BOOLEAN) {
        return 1;
    } else if (v4l2_query.type & V4L2_CTRL_TYPE_INTEGER) {
        return v4l2_query.step;
    }

    return 0;
}

