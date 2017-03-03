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

#include "CameraTool.h"
#ifdef LINUX
#include <pwd.h>
#endif

CameraConfig CameraTool::cam_cfg = {};
char CameraTool::cam_cfg_path[1024];

void CameraTool::printConfig(std::vector<CameraConfig> cfg_list) {

	if(cfg_list.size()==0) return;

	int device = -1;
	int width = -1;
	int height = -1;
	float fps = -1;
	int format = -1;
	int frame_mode = -1;

	for (int i=0;i<(int)cfg_list.size();i++) {

		if (cfg_list[i].device != device) {
			if (device>=0) printf("\b fps\n");
			device = cfg_list[i].device;
			printf("  %d: %s\n",cfg_list[i].device,cfg_list[i].name);
			format = -1;
		}

		if ((cfg_list[i].cam_format != format) || (cfg_list[i].frame_mode != frame_mode)) {
			if (format>=0) printf("\b fps\n");
			format = cfg_list[i].cam_format;
			if(cfg_list[i].frame_mode<0) printf("    format: %s",fstr[cfg_list[i].cam_format]);
			else printf("    format7_%d: %s",cfg_list[i].frame_mode,fstr[cfg_list[i].cam_format]);
			//if(cfg_list[i].compress) printf(" (default)");
			width = height = fps = -1;
			printf("\n");
		}

		if ((cfg_list[i].cam_width != width) || (cfg_list[i].cam_height != height)) {
			if (width>0) printf("\b fps\n");
			printf("      %dx%d ",cfg_list[i].cam_width,cfg_list[i].cam_height);
			width = cfg_list[i].cam_width;
			height = cfg_list[i].cam_height;
			fps = INT_MAX;
		}

		if (cfg_list[i].frame_mode>=0) printf("max|");
		else if (cfg_list[i].cam_fps != fps) {
			if(int(cfg_list[i].cam_fps)==cfg_list[i].cam_fps)
				printf("%d|",int(cfg_list[i].cam_fps));
			else printf("%.1f|",cfg_list[i].cam_fps);
			fps = cfg_list[i].cam_fps;
		}
	} printf("\b fps\n");

}

std::vector<CameraConfig> CameraTool::findDevices() {

#ifdef WIN32
	std::vector<CameraConfig> dev_list = videoInputCamera::getCameraConfigs();
#endif

#ifdef __APPLE__

#ifdef __x86_64__
	std::vector<CameraConfig> dev_list = AVfoundationCamera::getCameraConfigs();
#else
	std::vector<CameraConfig> dev_list = QTKitCamera::getCameraConfigs();
#endif

	std::vector<CameraConfig> dc1394_list = DC1394Camera::getCameraConfigs();
	dev_list.insert( dev_list.end(), dc1394_list.begin(), dc1394_list.end() );

#endif

#ifdef LINUX

	std::vector<CameraConfig> dev_list = V4Linux2Camera::getCameraConfigs();
	std::vector<CameraConfig> dc1394_list = DC1394Camera::getCameraConfigs();
	dev_list.insert( dev_list.end(), dc1394_list.begin(), dc1394_list.end() );

#endif

#ifndef LINUX
	std::vector<CameraConfig> ps3eye_list = PS3EyeCamera::getCameraConfigs();
	dev_list.insert(dev_list.end(), ps3eye_list.begin(), ps3eye_list.end());
#endif

	if(dev_list.size() > 1)
	{
		std::vector<CameraConfig> multicam_list = MultiCamera::getCameraConfigs(dev_list);
		dev_list.insert(dev_list.end(), multicam_list.begin(), multicam_list.end());
	}

	return dev_list;

}


void CameraTool::listDevices() {

	int dev_count;
#ifdef WIN32
	dev_count = videoInputCamera::getDeviceCount();
	if (dev_count==0) printf("no system camera found\n");
	else {
		if (dev_count==1) printf("1 system camera found:\n");
		else  printf("%d system cameras found:\n",dev_count);
		printConfig(videoInputCamera::getCameraConfigs());
	}
#endif

#ifdef __APPLE__
	dev_count = DC1394Camera::getDeviceCount();
	if (dev_count==0) printf("no DC1394 camera found\n");
	else {
		if (dev_count==1) printf("1 DC1394 camera found:\n");
		else  printf("%d DC1394 cameras found:\n",dev_count);
		printConfig(DC1394Camera::getCameraConfigs());
	}
#ifdef __x86_64__
	dev_count = AVfoundationCamera::getDeviceCount();
	if (dev_count==0) printf("no system camera found\n");
	else {
		if (dev_count==1) printf("1 system camera found:\n");
		else  printf("%d system cameras found:\n",dev_count);
		printConfig(AVfoundationCamera::getCameraConfigs());
	}
#else
	dev_count = QTKitCamera::getDeviceCount();
	if (dev_count==0) printf("no system camera found\n");
	else {
		if (dev_count==1) printf("1 system camera found:\n");
		else  printf("%d system cameras found:\n",dev_count);
		printConfig(QTKitCamera::getCameraConfigs());
	}
#endif
#endif

#ifdef LINUX
	dev_count = DC1394Camera::getDeviceCount();
	if (dev_count==0) printf("no DC1394 camera found\n");
	else {
		if (dev_count==1) printf("1 DC1394 camera found:\n");
		else  printf("%d DC1394 cameras found:\n",dev_count);
		printConfig(DC1394Camera::getCameraConfigs());
	}

	dev_count = V4Linux2Camera::getDeviceCount();
	if (dev_count==0) printf("no system camera found\n");
	else {
		if (dev_count==1) printf("1 system camera found:\n");
		else  printf("%d system cameras found:\n",dev_count);
		printConfig(V4Linux2Camera::getCameraConfigs());
	}
#endif

#ifndef LINUX
	dev_count = PS3EyeCamera::getDeviceCount();
	if(dev_count == 0) printf("no PS3Eye camera found\n");
	else {
		if(dev_count == 1) printf("1 PS3Eye camera found:\n");
		else  printf("%d PS3Eye cameras found:\n", dev_count);
		printConfig(PS3EyeCamera::getCameraConfigs());
	}
#endif
}

CameraEngine* CameraTool::getDefaultCamera() {

	initCameraConfig(&cam_cfg);

#ifdef WIN32
	return videoInputCamera::getCamera(&cam_cfg);
#endif

#ifdef __APPLE__
#ifdef __x86_64__
	return AVfoundationCamera::getCamera(&cam_cfg);
#else
	return QTKitCamera::getCamera(&cam_cfg);
#endif
#endif

#ifdef LINUX
	return V4Linux2Camera::getCamera(&cam_cfg);
#endif

	return NULL;
}

CameraEngine* CameraTool::getCamera(CameraConfig *cam_cfg, bool fallback) {

	CameraEngine* camera = NULL;
	int dev_count;

    if (cam_cfg->driver==DRIVER_MUTLICAM) {
		camera = MultiCamera::getCamera(cam_cfg);
		if (camera) return camera;
	}

#ifndef NDEBUG

	if (cam_cfg->driver==DRIVER_FILE) {
		camera = FileCamera::getCamera(cam_cfg);
		if (camera) return camera;
	}

	if (cam_cfg->driver==DRIVER_FOLDER) {
		camera = FolderCamera::getCamera(cam_cfg);
		if (camera) return camera;
	}

#endif

#ifndef LINUX
	if(cam_cfg->driver == DRIVER_PS3EYE) {
		dev_count = PS3EyeCamera::getDeviceCount();
		if(dev_count == 0) printf("no PS3Eye camera found\n");
		else camera = PS3EyeCamera::getCamera(cam_cfg);
		if(camera) return camera;
	}
#endif

#ifdef WIN32

	dev_count = videoInputCamera::getDeviceCount();
	if (dev_count==0) printf("no system camera found\n");
	else camera = videoInputCamera::getCamera(cam_cfg);
	if (camera) return camera;
	else if(fallback) return getDefaultCamera();

#endif

#ifdef __APPLE__

	if (cam_cfg->driver==DRIVER_DC1394) {
		dev_count = DC1394Camera::getDeviceCount();
		if (dev_count==0) printf("no DC1394 camera found\n");
		else camera = DC1394Camera::getCamera(cam_cfg);
		if (camera) return camera;
	}

#ifdef __x86_64__
	// default driver
	dev_count = AVfoundationCamera::getDeviceCount();
	if (dev_count==0) {
		printf("no system camera found\n");
		return NULL;
	} else camera = AVfoundationCamera::getCamera(cam_cfg);
	if (camera) return camera;
	else if(fallback) return getDefaultCamera();
#else
	// default driver
	dev_count = QTKitCamera::getDeviceCount();
	if (dev_count==0) {
		printf("no system camera found\n");
		return NULL;
	} else camera = QTKitCamera::getCamera(cam_cfg);
	if (camera) return camera;
	else if (fallback)return getDefaultCamera();
#endif
#endif

#ifdef LINUX

	if (cam_cfg->driver==DRIVER_DC1394) {
		dev_count = DC1394Camera::getDeviceCount();
		if (dev_count==0) printf("no DC1394 camera found\n");
		else camera = DC1394Camera::getCamera(cam_cfg);
		if (camera) return camera;
	}

	// default driver
	dev_count = V4Linux2Camera::getDeviceCount();
	if (dev_count==0) {
		printf("no system camera found\n");
		return NULL;
	} else camera = V4Linux2Camera::getCamera(cam_cfg);
	if (camera) return camera;
	else if(fallback) return getDefaultCamera();

#endif

	return NULL;
}

void CameraTool::initCameraConfig(CameraConfig *cfg) {

	sprintf(cfg->src,"none");
	cfg->name[0] = '\0';

	cfg->driver = DRIVER_DEFAULT;
	cfg->device = 0;
	cfg->cam_format = FORMAT_UNKNOWN;
	cfg->src_format = FORMAT_YUYV;
	cfg->buf_format = FORMAT_GRAY;

	cfg->cam_width = SETTING_MAX;
	cfg->cam_height = SETTING_MAX;
	cfg->cam_fps = SETTING_MAX;

	cfg->frame = false;
	cfg->frame_width = SETTING_MAX;
	cfg->frame_height = SETTING_MAX;
	cfg->frame_xoff = 0;
	cfg->frame_yoff = 0;
	cfg->frame_mode = -1;

	cfg->brightness = SETTING_DEFAULT;
	cfg->contrast = SETTING_DEFAULT;
	cfg->sharpness = SETTING_DEFAULT;
	cfg->gain = SETTING_DEFAULT;

	cfg->exposure = SETTING_DEFAULT;
	cfg->shutter = SETTING_DEFAULT;
	cfg->focus = SETTING_DEFAULT;
	cfg->white = SETTING_DEFAULT;
	cfg->gamma = SETTING_DEFAULT;
	cfg->powerline = SETTING_DEFAULT;
	cfg->backlight = SETTING_DEFAULT;

	cfg->hue = SETTING_DEFAULT;
	cfg->blue = SETTING_DEFAULT;
	cfg->red = SETTING_DEFAULT;
	cfg->green = SETTING_DEFAULT;

    cfg->color = false;
	cfg->force = false;
    cfg->flip_h = false;
    cfg->flip_v = false;

    cfg->calib_grid_path[0] = '\0';
}

void CameraTool::setCameraConfig(CameraConfig *cfg) {

	if ((cam_cfg.driver != cfg->driver) || (cam_cfg.device != cfg->device)) {

		cam_cfg.brightness = SETTING_DEFAULT;
		cam_cfg.contrast = SETTING_DEFAULT;
		cam_cfg.sharpness = SETTING_DEFAULT;
		cam_cfg.gain = SETTING_DEFAULT;

		cam_cfg.exposure = SETTING_DEFAULT;
		cam_cfg.shutter = SETTING_DEFAULT;
		cam_cfg.focus = SETTING_DEFAULT;
		cam_cfg.white = SETTING_DEFAULT;
		cam_cfg.gamma = SETTING_DEFAULT;
		cam_cfg.powerline = SETTING_DEFAULT;
		cam_cfg.backlight = SETTING_DEFAULT;

		cam_cfg.hue = SETTING_DEFAULT;
		cam_cfg.blue = SETTING_DEFAULT;
		cam_cfg.red = SETTING_DEFAULT;
		cam_cfg.green = SETTING_DEFAULT;
	}

	strncpy(cam_cfg.name, cfg->name, 256);

	cam_cfg.driver = cfg->driver;
	cam_cfg.device = cfg->device;
	cam_cfg.cam_format = cfg->cam_format;

	cam_cfg.cam_width = cfg->cam_width;
	cam_cfg.cam_height = cfg->cam_height;
	cam_cfg.cam_fps = cfg->cam_fps;

	cam_cfg.frame = cfg->frame;
    cam_cfg.frame_width = cfg->frame_width;
    cam_cfg.frame_height = cfg->frame_height;
    cam_cfg.frame_xoff = cfg->frame_xoff;
    cam_cfg.frame_yoff = cfg->frame_yoff;
	cam_cfg.frame_mode = cfg->frame_mode;

	cam_cfg.force = cfg->force;
    cam_cfg.flip_h = cfg->flip_h;
    cam_cfg.flip_v = cfg->flip_v;

	strncpy(cam_cfg.calib_grid_path, cfg->calib_grid_path, 1024);

    cam_cfg.childs = cfg->childs;
}

void CameraTool::whereIsConfig(const char* const cfgfilename, char* cfgfile) {

#ifdef __APPLE__
    char path[1024];
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle);
    CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
    CFStringGetCString( cfStringRef, path, 1024, kCFStringEncodingASCII);
    CFRelease( mainBundleURL);
    CFRelease( cfStringRef);
    sprintf(cfgfile,"%s/Contents/Resources/%s", path, cfgfilename);
#elif !defined WIN32
    sprintf(cfgfile, "./%s", cfgfilename);
    if(access(cfgfile, F_OK) == 0) return;

    sprintf(cfgfile, "/usr/share/portvideo/%s", cfgfilename);
    if(access(cfgfile, F_OK) == 0) return;

    sprintf(cfgfile, "/usr/local/share/portvideo/%s", cfgfilename);
    if(access(cfgfile, F_OK) == 0) return;

    sprintf(cfgfile, "/opt/share/portvideo/%s", cfgfilename);
    if(access(cfgfile, F_OK) == 0) return;

	sprintf(cfgfile, "/opt/local/share/portvideo/%s", cfgfilename);
	if(access(cfgfile, F_OK) == 0) return;

    *cfgfile = '\0';
#else
    sprintf(cfgfile, "./%s", cfgfilename);
#endif
}

int CameraTool::mapCameraDriver(const char* const driver) {

    if(driver == NULL) return DRIVER_DEFAULT;


    if(strcmp(driver, "dc1394" ) == 0) return DRIVER_DC1394;
    else if(strcmp(driver, "ps3eye" ) == 0) return DRIVER_PS3EYE;
    else if(strcmp(driver, "file" ) == 0) return DRIVER_FILE;
    else if(strcmp(driver, "folder" ) == 0) return DRIVER_FOLDER;
    else if(strcmp(driver, "multicam") == 0) return DRIVER_MUTLICAM;
    return DRIVER_DEFAULT;
}

CameraConfig* CameraTool::readSettings(const char* cfgfile) {

    if (strcmp(cfgfile, "default") == 0) {
        whereIsConfig("camera.xml", cam_cfg_path);
    } else {
		sprintf(cam_cfg_path, "%s", cfgfile);
	}

	tinyxml2::XMLDocument xml_settings;
	xml_settings.LoadFile(cam_cfg_path);
	if (xml_settings.Error()) {
		std::cout << "Error loading camera configuration file: '" << cam_cfg_path << "'" << std::endl;
		return &cam_cfg;
	}

	tinyxml2::XMLHandle docHandle( &xml_settings );
	tinyxml2::XMLHandle camera = docHandle.FirstChildElement("portvideo").FirstChildElement("camera");
	tinyxml2::XMLElement* camera_element = camera.ToElement();

    initCameraConfig(&cam_cfg);
    readSettings(camera_element, cam_cfg);
    return &cam_cfg;
}

void CameraTool::readSettings(tinyxml2::XMLElement* camera_element, CameraConfig& cam_cfg)
{
	if ( camera_element==NULL ) {
		std::cout << "Error loading camera configuration file: '" << cam_cfg_path << "'" << std::endl;
        return;
	}

    const char* driver = camera_element->Attribute("driver");
    cam_cfg.driver = mapCameraDriver(driver);

	if (camera_element->Attribute("id")!=NULL) {
		if (strcmp(camera_element->Attribute("id"), "auto" ) == 0) cam_cfg.device=SETTING_AUTO;
		else cam_cfg.device = atoi(camera_element->Attribute("id"));
	}

	if (camera_element->Attribute("src")!=NULL) {
#ifdef __APPLE__
		sprintf(cam_cfg.src,"%s/../%s",path,camera_element->Attribute("src"));
#else
		sprintf(cam_cfg.src,"%s",camera_element->Attribute("src"));
#endif
	}

	tinyxml2::XMLElement* image_element = camera_element->FirstChildElement("capture");

	if (image_element!=NULL) {
		if ((image_element->Attribute("color")!=NULL) && ( strcmp( image_element->Attribute("color"), "true" ) == 0 )) cam_cfg.color = true;

		if (image_element->Attribute("format")!=NULL) {
			for (int i=FORMAT_MAX;i>0;i--) {
				if (strcmp( image_element->Attribute("format"), fstr[i] ) == 0) cam_cfg.cam_format = i;
			}
		}

		if (image_element->Attribute("width")!=NULL) {
			if (strcmp( image_element->Attribute("width"), "max" ) == 0) cam_cfg.cam_width = SETTING_MAX;
			else if (strcmp( image_element->Attribute("width"), "min" ) == 0) cam_cfg.cam_width = SETTING_MIN;
			else cam_cfg.cam_width = atoi(image_element->Attribute("width"));
		}
		if (image_element->Attribute("height")!=NULL) {
			if (strcmp( image_element->Attribute("height"), "max" ) == 0) cam_cfg.cam_height = SETTING_MAX;
			else if (strcmp( image_element->Attribute("height"), "min" ) == 0) cam_cfg.cam_height = SETTING_MIN;
			else cam_cfg.cam_height = atoi(image_element->Attribute("height"));
		}
		if (image_element->Attribute("fps")!=NULL) {
			if (strcmp( image_element->Attribute("fps"), "max" ) == 0) cam_cfg.cam_fps = SETTING_MAX;
			else if (strcmp( image_element->Attribute("fps"), "min" ) == 0) cam_cfg.cam_fps = SETTING_MIN;
			else cam_cfg.cam_fps = atof(image_element->Attribute("fps"));
		}
		if ((image_element->Attribute("force")!=NULL) && ( strcmp( image_element->Attribute("force"), "true" ) == 0 )) cam_cfg.force = true;
        cam_cfg.flip_h = image_element->Attribute("flip_h") != NULL && strcmp(image_element->Attribute("flip_h"), "true") == 0;
        cam_cfg.flip_v = image_element->Attribute("flip_v") != NULL && strcmp(image_element->Attribute("flip_v"), "true") == 0;
	}

	tinyxml2::XMLElement* frame_element = camera_element->FirstChildElement("frame");
	if (frame_element!=NULL) {
		cam_cfg.frame = true;

		if (frame_element->Attribute("width")!=NULL) {
			if (strcmp( frame_element->Attribute("width"), "max" ) == 0) cam_cfg.frame_width = SETTING_MAX;
			else if (strcmp( frame_element->Attribute("width"), "min" ) == 0) cam_cfg.frame_width = 0;
			else cam_cfg.frame_width = atoi(frame_element->Attribute("width"));
		}

		if (frame_element->Attribute("height")!=NULL) {
			if (strcmp( frame_element->Attribute("height"), "max" ) == 0) cam_cfg.frame_height = SETTING_MAX;
			else if (strcmp( frame_element->Attribute("height"), "min" ) == 0) cam_cfg.frame_height = 0;
			else cam_cfg.frame_height = atoi(frame_element->Attribute("height"));
		}

		if (frame_element->Attribute("xoff")!=NULL) {
			if (strcmp( frame_element->Attribute("xoff"), "max" ) == 0) cam_cfg.frame_xoff = SETTING_MAX;
			else if (strcmp( frame_element->Attribute("xoff"), "min" ) == 0) cam_cfg.frame_xoff = 0;
			else cam_cfg.frame_xoff = atoi(frame_element->Attribute("xoff"));
		}

		if (frame_element->Attribute("yoff")!=NULL) {
			if (strcmp( frame_element->Attribute("yoff"), "max" ) == 0) cam_cfg.frame_yoff = SETTING_MAX;
			else if (strcmp( frame_element->Attribute("yoff"), "min" ) == 0) cam_cfg.frame_yoff = 0;
			else cam_cfg.frame_yoff = atoi(frame_element->Attribute("yoff"));
		}

		if (frame_element->Attribute("mode")!=NULL) {
			if (strcmp( frame_element->Attribute("mode"), "max" ) == 0) cam_cfg.frame_mode = SETTING_MAX;
			else if (strcmp( frame_element->Attribute("mode"), "min" ) == 0) cam_cfg.frame_mode = 0;
			else cam_cfg.frame_mode = atoi(frame_element->Attribute("mode"));
		}
	}

	tinyxml2::XMLElement* settings_element = camera_element->FirstChildElement("settings");
	if (settings_element!=NULL) {

		cam_cfg.brightness = readAttribute(settings_element, "brightness");
		cam_cfg.contrast = readAttribute(settings_element, "contrast");
		cam_cfg.sharpness = readAttribute(settings_element, "sharpness");
		cam_cfg.gain = readAttribute(settings_element, "gain");

		cam_cfg.exposure = readAttribute(settings_element, "exposure");
		cam_cfg.shutter = readAttribute(settings_element, "shutter");
		cam_cfg.focus = readAttribute(settings_element, "focus");
		cam_cfg.white = readAttribute(settings_element, "white");
		cam_cfg.powerline = readAttribute(settings_element, "powerline");
		cam_cfg.backlight = readAttribute(settings_element, "backlight");
		cam_cfg.gamma = readAttribute(settings_element, "gamma");

		if (cam_cfg.color) {
			cam_cfg.saturation= readAttribute(settings_element, "saturation");
			cam_cfg.hue = readAttribute(settings_element, "hue");
			cam_cfg.red = readAttribute(settings_element, "red");
			cam_cfg.green = readAttribute(settings_element, "green");
			cam_cfg.blue = readAttribute(settings_element, "blue");
		} else {
			cam_cfg.saturation = SETTING_OFF;
			cam_cfg.hue = SETTING_OFF;
			cam_cfg.red = SETTING_OFF;
			cam_cfg.green = SETTING_OFF;
			cam_cfg.blue = SETTING_OFF;
		}
	}

	tinyxml2::XMLElement* calibration_element = camera_element->FirstChildElement("calibration");
	if (calibration_element != NULL) {
		if (calibration_element->Attribute("file") != NULL) {
			if (calibration_element->Attribute("file") != NULL) {
				sprintf(cam_cfg.calib_grid_path, "%s", calibration_element->Attribute("file"));
			}
		}
	}


    cam_cfg.childs.clear();
	tinyxml2::XMLElement* child_cam = camera_element->FirstChildElement("child");
	while (child_cam != NULL) {
		CameraConfig child_cfg;
		initCameraConfig(&child_cfg);
		readSettings(child_cam, child_cfg);
		cam_cfg.childs.push_back(child_cfg);
		child_cam = child_cam->NextSiblingElement("child");
	}
}

int CameraTool::readAttribute(tinyxml2::XMLElement* settings,const char *attribute) {

	if(settings->Attribute(attribute)!=NULL) {
		if (strcmp(settings->Attribute(attribute), "min" ) == 0) return SETTING_MIN;
		else if (strcmp(settings->Attribute(attribute), "max" ) == 0) return SETTING_MAX;
		else if (strcmp(settings->Attribute(attribute), "auto" ) == 0) return SETTING_AUTO;
		else if (strcmp(settings->Attribute(attribute), "default" ) == 0) return SETTING_DEFAULT;
		else return atoi(settings->Attribute(attribute));
	}

	return SETTING_DEFAULT;
}

void CameraTool::saveSettings() {

	tinyxml2::XMLDocument xml_settings;
	tinyxml2::XMLHandle docHandle( &xml_settings );

    tinyxml2::XMLElement* portvideo = xml_settings.NewElement("portvideo");
    xml_settings.LinkEndChild(portvideo);

    tinyxml2::XMLElement* camera_element = xml_settings.NewElement("camera");
    portvideo->LinkEndChild(camera_element);

    saveSettings(cam_cfg, camera_element);
	xml_settings.SaveFile(cam_cfg_path);
	if (xml_settings.Error()) std::cout << "Error saving camera configuration file: "  << cam_cfg_path << std::endl;
}

void CameraTool::saveSettings(CameraConfig& cam_cfg, tinyxml2::XMLElement* camera_element)
{
    tinyxml2::XMLDocument* doc = camera_element->GetDocument();

	camera_element->SetAttribute("driver",dstr[cam_cfg.driver]);
	camera_element->SetAttribute("id",cam_cfg.device);

	tinyxml2::XMLElement* image_element = doc->NewElement("capture");
	image_element->SetAttribute("format",fstr[cam_cfg.cam_format]);
	image_element->SetAttribute("width",cam_cfg.cam_width);
	image_element->SetAttribute("height",cam_cfg.cam_height);
	image_element->SetAttribute("fps",cam_cfg.cam_fps);
    camera_element->LinkEndChild(image_element);

	tinyxml2::XMLElement* settings_element = doc->NewElement("settings");
	if (shouldSaveAttribute(cam_cfg.brightness)) saveAttribute(settings_element, "brightness", cam_cfg.brightness);
	if (shouldSaveAttribute(cam_cfg.contrast)) saveAttribute(settings_element, "contrast", cam_cfg.contrast);
	if (shouldSaveAttribute(cam_cfg.sharpness)) saveAttribute(settings_element, "sharpness", cam_cfg.sharpness);
	if (shouldSaveAttribute(cam_cfg.gain)) saveAttribute(settings_element, "gain", cam_cfg.gain);

	if (shouldSaveAttribute(cam_cfg.exposure)) saveAttribute(settings_element, "exposure", cam_cfg.exposure);
	if (shouldSaveAttribute(cam_cfg.focus)) saveAttribute(settings_element, "focus", cam_cfg.focus);
	if (shouldSaveAttribute(cam_cfg.shutter)) saveAttribute(settings_element, "shutter", cam_cfg.shutter);
	if (shouldSaveAttribute(cam_cfg.white)) saveAttribute(settings_element, "white", cam_cfg.white);
	if (shouldSaveAttribute(cam_cfg.backlight)) saveAttribute(settings_element, "backlight", cam_cfg.backlight);
	if (shouldSaveAttribute(cam_cfg.powerline)) saveAttribute(settings_element, "powerline", cam_cfg.powerline);
	if (shouldSaveAttribute(cam_cfg.gamma)) saveAttribute(settings_element, "gamma", cam_cfg.gamma);

	if (shouldSaveAttribute(cam_cfg.saturation)) saveAttribute(settings_element, "saturation", cam_cfg.saturation);
	if (shouldSaveAttribute(cam_cfg.hue)) saveAttribute(settings_element, "hue", cam_cfg.hue);
	if (shouldSaveAttribute(cam_cfg.red)) saveAttribute(settings_element, "red", cam_cfg.red);
	if (shouldSaveAttribute(cam_cfg.green)) saveAttribute(settings_element, "green", cam_cfg.green);
	if (shouldSaveAttribute(cam_cfg.blue)) saveAttribute(settings_element, "blue", cam_cfg.green);
    camera_element->LinkEndChild(settings_element);

	tinyxml2::XMLElement* frame_element = doc->NewElement("frame");
	if (cam_cfg.frame_mode>=0) saveAttribute(frame_element, "mode", cam_cfg.frame_mode);
    camera_element->LinkEndChild(frame_element);

	tinyxml2::XMLElement* calibration_element = doc->NewElement("calibration");
	calibration_element->SetAttribute("file", cam_cfg.calib_grid_path);
    camera_element->LinkEndChild(calibration_element);

    std::vector<CameraConfig>::iterator iter;
    for(iter = cam_cfg.childs.begin(); iter != cam_cfg.childs.end(); iter++)
    {
        tinyxml2::XMLElement* child_cam_element = doc->NewElement("child");
        saveSettings(*iter, child_cam_element);
        camera_element->LinkEndChild(child_cam_element);
    }

    if(!settings_element->NoChildren())
        camera_element->DeleteChild(settings_element);
    if(!frame_element->NoChildren())
        camera_element->DeleteChild(frame_element);

}

void CameraTool::saveAttribute(tinyxml2::XMLElement* settings,const char *attribute,int config) {

	if (config==SETTING_MIN) settings->SetAttribute(attribute,"min");
	else if (config==SETTING_MAX) settings->SetAttribute(attribute,"max");
	else if (config==SETTING_AUTO) settings->SetAttribute(attribute,"auto");
	else {
		char value[64];
		sprintf(value,"%d",config);
		settings->SetAttribute(attribute,value);
	}
}

bool CameraTool::shouldSaveAttribute(int config)
{
    return config != SETTING_OFF && config != SETTING_DEFAULT;
}
