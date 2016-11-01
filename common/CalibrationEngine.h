/*  reacTIVision tangible interaction framework
	Copyright (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>
 
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

#ifndef CALIBRATIONENGINE_H
#define CALIBRATIONENGINE_H

#include "FrameProcessor.h"
#include "FidtrackFinder.h"
#include "CalibrationGrid.h"
#include "VisionEngine.h"
#include <vector>

struct PointPair {
	
	FloatPoint original;
	FloatPoint rotation;
	float angle;
};

typedef struct FidtrackFinderSettingBackup
{
    bool detect_fingers;
    bool detect_blobs;
    int min_blob_size;
    int max_blob_size;
} FidtrackFinderSettingBackup;

typedef struct DistortionCalibData
{
    int cur_x;
    int cur_y;
    int count_x;
    int count_y;

    float spacing_x;
    float spacing_y;
    bool blob_submitted;
} DistortionCalibData;

enum CalibrationStep {
    POSITION,
    BOUNDING,
    DISTORTION,
    TESTRESULT,
};

class CalibrationEngine: public FrameProcessor
{
public:
	CalibrationEngine(TuioServer* manager, FidtrackFinder* fidtrack_finder, VisionEngine* engine, application_settings* config);
	~CalibrationEngine();

    void process(unsigned char *src, unsigned char *dest);
	
	bool setFlag(unsigned char flag, bool value, bool lock);
	bool toggleFlag(unsigned char flag, bool lock);
	
private:
    TuioServer* tuio_mgr;
    VisionEngine* engine;
    FidtrackFinder* fid_finder;
	CalibrationGrid *grid;
    char calib_out[1024];
    application_settings* config;
    CameraConfig* cam_cfg;

	char calib_bak[1024];
	bool file_exists;
	
	bool calibration;
	
	int frame_xoff, frame_yoff;
	int frame_width, frame_height;
	
	DisplayMode prevMode;

    int cur_cam_idx;
    CalibrationStep step;
    DistortionCalibData calib_data;
    FidtrackFinderSettingBackup fid_finder_bak;
    CameraConfig cam_cfg_bak;

    void setupStepPosition();
    void commitStepPosition(int old_cam_idx);
    void setupStepBounding();
    void commitStepBounding();
    void setupStepDistortion();
    void commitStepDistortion();
    void setupStepTestResult();

    void backupFidFinderSettings();
    void restoreFidFinderSettings();

    void getGridConfigPath(char* config_path);
    TUIO::TuioBlob* getFingerPressedBlob();
    int getCamIndexByPoint(TUIO::TuioPoint x_Point);
};

#endif
