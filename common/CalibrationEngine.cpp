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

#include "CalibrationEngine.h"
#ifdef __APPLE__
#include <CoreFoundation/CFBundle.h>
#elif defined  LINUX
#include <unistd.h>
#endif

CalibrationEngine::CalibrationEngine(TuioServer* manager, FidtrackFinder* fidtrack_finder, VisionEngine* vision_engine, application_settings* app_config) {
	
    tuio_mgr = manager;
    fid_finder = fidtrack_finder;
    engine = vision_engine;
	calibration = false;
	file_exists = false;
    grid = NULL;
    config = app_config;
    cur_cam_idx = 0;
	
#ifdef WIN32
	FILE *infile = fopen (calib_out, "r");
	if (infile != NULL) {
		file_exists = true;
		fclose (infile);
	}
#else
	file_exists = (access (calib_out, F_OK )==0);
#endif
	
	help_text.push_back( "CalibrationEngine:");
	help_text.push_back( "   c - toggle calibration");
	help_text.push_back( "   q - toggle quick/precise mode");
	help_text.push_back( "   u - reset selected point");
	help_text.push_back( "   j - reset calibration grid");
	help_text.push_back( "   l - revert to saved grid");
	help_text.push_back( "   r - show calibration result");
	help_text.push_back( "   a,d,w,x - move within grid");
	help_text.push_back( "   cursor keys - adjust grid point");
};

CalibrationEngine::~CalibrationEngine() {
	if (grid != NULL) delete grid;
	
	if (file_exists) {
		remove(calib_bak);
	}
};

bool CalibrationEngine::setFlag(unsigned char flag, bool value, bool lock) {
	return toggleFlag(flag,lock);
}

bool CalibrationEngine::toggleFlag(unsigned char flag, bool lock) {
	
	if (flag==KEY_C) {
		if (calibration) {
			calibration = false;

            //restore
            engine->teardownCamera();
            engine->resetCamera(&cam_cfg_bak);
            restoreFidFinderSettings();

			if(ui) ui->setDisplayMode(prevMode);
		} else {
			calibration = true;

            cam_cfg = CameraTool::readSettings(config->camera_config);
            cam_cfg_bak = *cam_cfg;

            backupFidFinderSettings();

            if(cam_cfg->childs.size() > 0)
                setupStepPosition();
            else
                setupStepBounding();


			if(ui) {
				prevMode = ui->getDisplayMode();
				ui->setDisplayMode(SOURCE_DISPLAY);
			}
		}
	}

    if(!calibration) return lock;

    if(flag == KEY_ENTER || flag == KEY_ENTER_RIGHT)
    {
        if(step == CalibrationStep::BOUNDING)
        {
            commitStepBounding();
            setupStepDistortion();
        }
    }
    else if(step == CalibrationStep::BOUNDING)
    {
        if(flag == KEY_D)
        {
            if(frame_xoff < width - 10)
            {
                frame_xoff++;
                frame_width--;
            }
        }
        else if(flag == KEY_S)
        {
            if(frame_yoff < height - 10)
            {
                frame_yoff++;
                frame_height--;
            }
        }
        else if(flag == KEY_A)
        {
            if(frame_xoff > 0)
            {
                frame_xoff--;
                frame_width++;
            }
        }
        else if(flag == KEY_W)
        {
            if(frame_yoff > 0)
            {
                frame_yoff--;
                frame_height++;
            }
        }
        else if(flag == KEY_RIGHT)
        {
            if(frame_width < width - frame_xoff) frame_width++;
        }
        else if(flag == KEY_LEFT)
        {
            if(frame_width > 10) frame_width--;
        }
        else if(flag == KEY_UP)
        {
            if(frame_height > 10) frame_height--;
        }
        else if(flag == KEY_DOWN)
        {
            if(frame_height < height - frame_yoff) frame_height++;
        }
    }
	
    return lock;
}

void CalibrationEngine::process(unsigned char *src, unsigned char *dest) {

    if(!calibration)
        return;

    int left, top, right, bottom = 0;

    left = frame_xoff;
    top = frame_yoff;

    if(cam_cfg->childs.size() > 0)
    {
        left += width / cam_cfg->childs[0].cam_width * cur_cam_idx;
        top += height / cam_cfg->childs[0].cam_height * cur_cam_idx;
    }

    right = left + frame_width - 1;
    bottom = top + frame_height - 1;

    //ui->setColor(0, 0, 0);
    //ui->fillRect(0, 0, width, height);

    if(step == CalibrationStep::POSITION)
    {
        assert(cam_cfg->childs.size() > 0);

        TuioBlob* blob = getFingerPressedBlob();
        int color_fade = 0;
        if(blob != NULL)
        {
            int cursor_duration_sec = (TuioTime::getSystemTime() - blob->getStartTime()).getSeconds();
            color_fade = 80 * (cursor_duration_sec + 1);

            if(cursor_duration_sec >= 3)
            {
                TUIO::TuioPoint pos = blob->getPosition();
                int old_cam_idx = getCamIndexByPoint(pos);
                commitStepPosition(old_cam_idx);
                setupStepBounding();
            }
        }

        CameraConfig firstChildCam = cam_cfg->childs[0];
        int cam_rows = height / firstChildCam.cam_height;
        int cur_cam_col = cur_cam_idx % cam_rows;
        int cur_cam_row = cur_cam_idx / cam_rows;

        int x = (cur_cam_col + 0.5) * firstChildCam.cam_width;
        int y = (cur_cam_row + 0.5) * firstChildCam.cam_height;

        ui->setColor(0, 255, 0);
        ui->fillEllipse(x, y, 25, 25);
    }
    else if(step == CalibrationStep::BOUNDING)
    {
        ui->setColor(0, 255, 0);
        ui->drawLine(left, top, right, top);
        ui->drawLine(right, top, right, bottom);
        ui->drawLine(right, bottom, left, bottom);
        ui->drawLine(left, bottom, left, top);
        ui->setColor(0, 0, 255);
        ui->drawLine(left, top, right, bottom);
        ui->drawLine(right, top, left, bottom);
    }
    else if(step == CalibrationStep::DISTORTION)
    {
        //TODO: identify flipping
        TuioBlob* blob = getFingerPressedBlob();
        int cur_point_color_fade = 0;

        if(blob == NULL)
            calib_data.blob_submitted = false;
        else if(!calib_data.blob_submitted)
        {
            int cursor_duration_sec = (TuioTime::getSystemTime() - blob->getStartTime()).getSeconds();
            cur_point_color_fade = 80 * (cursor_duration_sec + 1);

            if(!calib_data.blob_submitted && cursor_duration_sec >= 3)
            {
                TUIO::TuioPoint pos = blob->getPosition();
                grid->Set(calib_data.cur_x, calib_data.cur_y, pos.getX(), pos.getY());

                calib_data.cur_x++;
                if(calib_data.cur_x >= calib_data.count_x)
                {
                    calib_data.cur_x = 0;
                    calib_data.cur_y++;
                }

                calib_data.blob_submitted = true;
            }
        }

        if(calib_data.cur_y >= calib_data.count_y)
        {
            if(cur_cam_idx >= cam_cfg->childs.size())
            {
                commitStepDistortion();
                setupStepTestResult();
            }
            else
            {
                cur_cam_idx++;
                setupStepPosition();
            }
        }

        for(int point_x = 0; point_x < calib_data.count_x; point_x++)
        {
            for(int point_y = 0; point_y < calib_data.count_y; point_y++)
            {
                GridPoint gp = grid->Get(point_x, point_y);
                if(point_x == calib_data.cur_x && point_y == calib_data.cur_y)
                    ui->setColor(255 - cur_point_color_fade, 0, cur_point_color_fade);
                else if(gp.x == 0 && gp.y == 0)
                    ui->setColor(0, 255, 0);
                else
                    ui->setColor(0, 0, 255);

                ui->fillEllipse(left + point_x * calib_data.spacing_x, top + point_y * calib_data.spacing_y, 25, 25);
            }
        }
    }

    //TODO: Display instruction message per calibrationstep
}

int CalibrationEngine::getCamIndexByPoint(TUIO::TuioPoint x_Point)
{
    if(cam_cfg->childs.size() <= 0)
        return 0;

    CameraConfig firstChildCam = cam_cfg->childs[0];
    int cam_x = x_Point.getX() / firstChildCam.cam_height;
    int cam_y = x_Point.getY() / firstChildCam.cam_width;

    return cam_y * width + cam_x;
}

TUIO::TuioBlob* CalibrationEngine::getFingerPressedBlob()
{
    std::list<TuioBlob*> blobs = tuio_mgr->getTuioBlobs();
    if(blobs.size() == 1)
        return *blobs.begin();

    return NULL;
}

void CalibrationEngine::setupStepTestResult()
{
    step = CalibrationStep::TESTRESULT;
}

void CalibrationEngine::setupStepBounding()
{
    step = CalibrationStep::BOUNDING;

    if(cam_cfg->frame)
    {
        cam_cfg->frame = false;
        cam_cfg->frame_height = SETTING_MAX;
        cam_cfg->frame_width = SETTING_MAX;
        cam_cfg->frame_xoff = 0;
        cam_cfg->frame_yoff = 0;
        engine->teardownCamera();
        engine->resetCamera(cam_cfg);
    }

    frame_xoff = 0;
    frame_yoff = 0;

    if(cam_cfg->childs.size() > 0)
    {
        frame_width = cam_cfg->childs[0].cam_width;
        frame_height = cam_cfg->childs[0].cam_height;
    }
    else
    {
        frame_width = width;
        frame_height = height;
    }
}

void CalibrationEngine::commitStepBounding()
{
    if(frame_xoff > 0 || frame_yoff > 0 || frame_width != width || frame_height != height)
    {
        cam_cfg->frame = true;
        cam_cfg->frame_xoff = frame_xoff;
        cam_cfg->frame_yoff = frame_yoff;
        cam_cfg->frame_width = frame_width;
        cam_cfg->frame_height = frame_height;
        engine->teardownCamera();
        engine->resetCamera(cam_cfg);
    }
}

void CalibrationEngine::setupStepDistortion()
{
    step = CalibrationStep::DISTORTION;

    getGridConfigPath(calib_out);

    frame_xoff = 0;
    frame_yoff = 0;
    frame_width = width;
    frame_height = height;

    grid = new CalibrationGrid(frame_width, frame_height);
    grid->Reset();

    calib_data.count_x = 6;
    calib_data.count_y = 6;
    if(((float)width / height) > 1.3) calib_data.count_x += 2;
    if(((float)width / height) > 1.7) calib_data.count_x += 2;
    calib_data.spacing_x = (frame_width - 1) / (float)(calib_data.count_x - 1);
    calib_data.spacing_y = (frame_height - 1) / (float)(calib_data.count_y - 1);

    calib_data.cur_x = 0;
    calib_data.cur_y = 0;
    calib_data.blob_submitted = false;

    //enable blob detecting and disable finger detecting
    //-> finger detecting wont work in high distorted areas
    fid_finder->detect_blobs = true;
    fid_finder->detect_fingers = false;
    fid_finder->min_blob_size = 10;
    fid_finder->max_blob_size = 100;
}

void CalibrationEngine::commitStepDistortion()
{
    grid->Store(calib_out);
}

void CalibrationEngine::setupStepPosition()
{
    step = CalibrationStep::POSITION;
}

void CalibrationEngine::commitStepPosition(int old_cam_idx)
{
    std::iter_swap(cam_cfg->childs.begin() + cur_cam_idx, cam_cfg->childs.begin() + old_cam_idx);
}

void CalibrationEngine::backupFidFinderSettings()
{
    fid_finder_bak.detect_blobs = fid_finder->detect_blobs;
    fid_finder_bak.detect_fingers = fid_finder->detect_fingers;
    fid_finder_bak.min_blob_size = fid_finder->min_blob_size;
    fid_finder_bak.max_blob_size = fid_finder->max_blob_size;
}

void CalibrationEngine::restoreFidFinderSettings()
{
    fid_finder->detect_blobs = fid_finder_bak.detect_blobs;
    fid_finder->detect_fingers = fid_finder_bak.detect_fingers;
    fid_finder->min_blob_size = fid_finder_bak.min_blob_size;
    fid_finder->max_blob_size = fid_finder_bak.max_blob_size;
}

void CalibrationEngine::getGridConfigPath(char* config_path)
{
#ifdef __APPLE__
    char path[1024];
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
    CFStringRef cfStringRef = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);
    CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);
    CFRelease(mainBundleURL);
    CFRelease(cfStringRef);
    sprintf(config_path, "%s/Contents/Resources/calibration.grid", path);
#else
    strcpy(config_path, "./calibration.grid");
#endif
}



