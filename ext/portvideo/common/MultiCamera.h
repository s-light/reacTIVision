/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2015 Martin Kaltenbrunner <martin@tuio.org>

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

#ifndef MULTICAMERA_H
#define	MULTICAMERA_H

#include <vector>
#include "CameraEngine.h"
#include "CameraTool.h"
#include "tinyxml2.h"

class MultiCamConfig
{
public:
    static std::vector<MultiCamConfig> readConfig(const char* const cam_cfg_file);
    static void writeConfig(std::vector<MultiCamConfig> cam_config, const char* const cam_cfg_file);
    
    static bool compareCol(const MultiCamConfig& x, const MultiCamConfig& y) { 
        return x.grid_col < y.grid_col; 
    };
    static bool compareRow(const MultiCamConfig& x, const MultiCamConfig& y) {
        return x.grid_row < y.grid_row;
    }
    
    
    MultiCamConfig();
    virtual ~MultiCamConfig() { }
    
    int driver;
    int device;
    
    int capture_width;
    int capture_height;
    
    int frame_width;
    int frame_height;
    int frame_xoff;
    int frame_yoff;
    
    int grid_row;
    int grid_col;
    
    CameraConfig* getChildCameraConfig(CameraConfig* cam_cfg);
    
private:
    CameraConfig child_cam_config_;
    
};

class MultiCamera : public CameraEngine
{
public:
    MultiCamera(CameraConfig *cam_cfg);
    virtual ~MultiCamera();
    
    static CameraEngine* getCamera(CameraConfig* cam_cfg);
    
	bool initCamera();
    bool closeCamera();
	unsigned char* getFrame();
	bool stillRunning() { return running; }
    
    bool startCamera();
	bool stopCamera();
	bool resetCamera();
    void printInfo();
    
	int getCameraSettingStep(int mode);
	int getCameraSetting(int mode);
	int getMaxCameraSetting(int mode);
	int getMinCameraSetting(int mode);
    bool getCameraSettingAuto(int mode);
    int getDefaultCameraSetting(int mode);
	bool hasCameraSetting(int mode);
	bool hasCameraSettingAuto(int mode);
    
	bool setCameraSettingAuto(int mode, bool flag);
	bool setCameraSetting(int mode, int value);
	bool setDefaultCameraSetting(int mode);

private:
    std::vector<MultiCamConfig> cam_config_;
    std::vector<CameraEngine*> cameras_;
    int cameras_columns_;
    int cameras_rows_;
    
    char cam_config_file_[1024];
    
    void setupChildCameras();
    bool checkMultiCamConfig();
    
};

#endif	/* MULTICAMERA_H */

