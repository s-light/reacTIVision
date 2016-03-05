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

#include "MultiCamera.h"

MultiCamConfig::MultiCamConfig() 
{
    driver = DRIVER_DEFAULT;
    device = 0;
    capture_width = 0;
    capture_height = 0;
    frame_height = 0;
    frame_width = 0;
    frame_xoff = 0;
    frame_yoff = 0;
    grid_col = 0;
    grid_row = 0;
    CameraTool::initCameraConfig(&child_cam_config_);
}

std::vector<MultiCamConfig> MultiCamConfig::readConfig(const char* const cam_cfg_file)
{
    //TODO: support max and min
    std::vector<MultiCamConfig> multicam_config;
    
    tinyxml2::XMLDocument doc;
    doc.LoadFile(cam_cfg_file);
    if(doc.Error())
    {
        std::cout << "Error loading multicam configuration file: " << cam_cfg_file << std::endl;
        return multicam_config;
    }
    
    tinyxml2::XMLHandle doc_handle(&doc);
    tinyxml2::XMLElement* camera_element = doc_handle.FirstChildElement("multicam").FirstChildElement("camera").ToElement();
    
    while(camera_element != NULL)
    {
        MultiCamConfig cfg;

        const char* driver = camera_element->Attribute("driver");
        cfg.driver = CameraTool::mapCameraDriver(driver);

        if(camera_element->Attribute("id") != NULL)
            cfg.device = atoi(camera_element->Attribute("id"));        
        
        tinyxml2::XMLElement* capture_element = camera_element->FirstChildElement("capture");
        if(capture_element != NULL)
        {   
            if(capture_element->Attribute("width") != NULL)
                cfg.capture_width = atoi(capture_element->Attribute("width"));
            if(capture_element->Attribute("height") != NULL)
                cfg.capture_height = atoi(capture_element->Attribute("height"));
        }
        
        tinyxml2::XMLElement* frame_element = camera_element->FirstChildElement("frame");
        if(frame_element != NULL)
        {
            if(frame_element->Attribute("height") != NULL)
                cfg.frame_height = atoi(frame_element->Attribute("height"));
            if(frame_element->Attribute("width") != NULL)
                cfg.frame_width = atoi(frame_element->Attribute("width"));
            if(frame_element->Attribute("xoff") != NULL)
                cfg.frame_xoff = atoi(frame_element->Attribute("xoff"));
            if(frame_element->Attribute("yoff") != NULL)
                cfg.frame_yoff = atoi(frame_element->Attribute("yoff"));
        }
        
        tinyxml2::XMLElement* grid_element = camera_element->FirstChildElement("grid");
        if(grid_element != NULL)
        {
            if(grid_element->Attribute("col") != NULL)
                cfg.grid_col = atoi(grid_element->Attribute("col"));
            if(grid_element->Attribute("row") != NULL)
                cfg.grid_row = atoi(grid_element->Attribute("row"));
        }
        
        multicam_config.push_back(cfg);
        camera_element = camera_element->NextSiblingElement("camera");
    }
    
    return multicam_config;
}

void MultiCamConfig::writeConfig(std::vector<MultiCamConfig> cam_config, const char* const cam_cfg_file)
{
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLHandle doc_handle(&doc);
    tinyxml2::XMLElement* doc_element = doc.ToElement();
    
    doc_element->DeleteChildren();
    
    tinyxml2::XMLElement* multicam_element = doc.NewElement("multicam");
    doc_element->LinkEndChild(multicam_element);
    
    std::vector<MultiCamConfig>::iterator iter;
    for(iter = cam_config.begin(); iter != cam_config.end(); iter++)
    {
        MultiCamConfig cfg = *iter;
        
        tinyxml2::XMLElement* camera_element = doc.NewElement("camera");
        camera_element->SetAttribute("driver", cfg.driver);
        camera_element->SetAttribute("device", cfg.device);
        
        tinyxml2::XMLElement* capture_element = doc.NewElement("capture");
        if(cfg.capture_width > 0) capture_element->SetAttribute("width", cfg.capture_width);
        if(cfg.capture_height > 0) capture_element->SetAttribute("height", cfg.capture_height);

        tinyxml2::XMLElement* frame_element = doc.NewElement("frame");
        if(cfg.frame_height > 0) frame_element->SetAttribute("height", cfg.frame_height);
        if(cfg.frame_width > 0) frame_element->SetAttribute("width", cfg.frame_width);
        if(cfg.frame_xoff > 0) frame_element->SetAttribute("xoff", cfg.frame_xoff);
        if(cfg.frame_yoff > 0) frame_element->SetAttribute("yoff", cfg.frame_yoff);
        
        tinyxml2::XMLElement* gird_element = doc.NewElement("grid");
        if(cfg.grid_col > 0) gird_element->SetAttribute("col", cfg.grid_col);
        if(cfg.grid_row > 0) gird_element->SetAttribute("row", cfg.grid_row);
        
        multicam_element->LinkEndChild(camera_element);
    }
    
    doc.SaveFile(cam_cfg_file);
    if(doc.Error())
        std::cout << "Error saving camera configuration file: " << cam_cfg_file << std::endl;
}

CameraConfig* MultiCamConfig::getChildCameraConfig(CameraConfig* cam_cfg)
{
    //take global camera config and override with per cam settings in multicam.xml
    
    child_cam_config_ = *cam_cfg;
    child_cam_config_.driver = driver;
    child_cam_config_.device = device;
    child_cam_config_.cam_width = capture_width;
    child_cam_config_.cam_height = capture_height;
    child_cam_config_.frame = true;
    child_cam_config_.frame_width = frame_width;
    child_cam_config_.frame_height = frame_height;
    child_cam_config_.frame_xoff = frame_xoff;
    child_cam_config_.frame_yoff = frame_yoff;
    
    if(child_cam_config_.driver == DRIVER_MUTLICAM)
        child_cam_config_.driver = DRIVER_DEFAULT;
    
    return &child_cam_config_;
}

CameraEngine* MultiCamera::getCamera(CameraConfig* cam_cfg)
{
    MultiCamera *cam = new MultiCamera(cam_cfg);
    if(!cam->checkMultiCamConfig())
    {
        delete cam;
        return NULL;
    }
    
    cam->setupChildCameras();
    return cam;
}

MultiCamera::MultiCamera(CameraConfig *cam_cfg) : CameraEngine(cam_cfg) 
{
    CameraTool::whereIsConfig("multicam.xml", cam_config_file_);
    cam_config_ = MultiCamConfig::readConfig(cam_config_file_);
    frm_buffer = NULL;
    cam_buffer = NULL;
}

MultiCamera::~MultiCamera() {
    
    std::vector<CameraEngine*>::iterator iter;
    for(iter = cameras_.begin(); iter != cameras_.end(); iter++)
        delete *iter;
    
    if (cam_buffer)
    {
        delete[] cam_buffer;
        cam_buffer = NULL;
    }
}

void MultiCamera::setupChildCameras()
{
    cameras_.resize(cameras_columns_ * cameras_rows_, NULL);
    
    std::vector<MultiCamConfig>::iterator iter;
    for(iter = cam_config_.begin(); iter != cam_config_.end(); iter++)
    {
        CameraConfig* child_cfg = iter->getChildCameraConfig(this->cfg);
        //TODO: if the spezified child camera can not be found, the default camera is used.
        // In doing so CameraTool resets the main camera config
        CameraEngine* cam = CameraTool::getCamera(child_cfg);
        int add_cam_at = iter->grid_row * cameras_columns_ + iter->grid_col;
        cameras_[add_cam_at] = cam;
    }
}

bool MultiCamera::checkMultiCamConfig()
{
    if(cam_config_.size() <= 0)
    {
        std::cout << "no child cameras spezified in multicam.xml" << std::endl;
        return false;
    }
    
    int cols = std::max_element(cam_config_.begin(), cam_config_.end(), MultiCamConfig::compareCol)->grid_col + 1;
    int rows = std::max_element(cam_config_.begin(), cam_config_.end(), MultiCamConfig::compareRow)->grid_row + 1;
    
    bool cams_exists[cols * rows];
    int height_per_col[cols];
    int width_per_row[rows];
    
    memset(cams_exists, 0, sizeof(bool)*cols*rows);
    memset(height_per_col, 0, sizeof(int)*cols);
    memset(width_per_row, 0, sizeof(int)*rows);
    
    std::vector<MultiCamConfig>::iterator iter;
    for(iter = cam_config_.begin(); iter != cam_config_.end(); iter++)
    {
        if(iter->frame_height <= 0 || iter->frame_width <= 0)
        {
            std::cout << "bad multicam layout: invalid frame_size" << std::endl;
            return false;
        }
        
        if(iter->frame_xoff >= iter->frame_width || iter->frame_yoff >= iter->frame_height)
        {
            std::cout << "bad multicam layout: invalid offset" << std::endl;
            return false;
        }
        
        int cam_exists_at = iter->grid_row * cols + iter->grid_col;
        if(cams_exists[cam_exists_at])
        {
            std::cout << "bad multicam layout: two or more cams placed in same cell" << std::endl;
            return false;
        }
        
        cams_exists[cam_exists_at] = true;
        
        height_per_col[iter->grid_col] = iter->frame_height;
        width_per_row[iter->grid_row] = iter->frame_width;
    }
    
    for(int i = 1; i < cols; i++)
    {
        if(height_per_col[i-1] != height_per_col[i])
        {
            std::cout << "bad multicam layout: different heights per column" << std::endl;
            return false;
        }
    }
    
    for(int i = 1; i < cols; i++)
    {
        if(height_per_col[i-1] != height_per_col[i])
        {
            std::cout << "bad multicam layout: different widths per row" << std::endl;
            return false;
        }
    }
    
    for(int i=0; i<cols * rows; i++)
    {
        if(!cams_exists[i])
        {
            std::cout << "bad multicam layout: gab in camera grid" << std::endl;
            return false;
        }
    }
    
    cameras_columns_ = cols;
    cameras_rows_ = rows;
    cfg->cam_height = height_per_col[0] * rows;
    cfg->cam_width = width_per_row[0] * cols;
    cfg->frame = false;
    setupFrame();
    
    return true;
}

void MultiCamera::printInfo()
{
    printf("Multicam: %i childs\n", (int)cameras_.size());
    
    std::vector<CameraEngine*>::iterator iter;
    for(iter = cameras_.begin(); iter != cameras_.end(); iter++)
    {
        (*iter)->printInfo();
        printf("\n");
    }
}

bool MultiCamera::initCamera() 
{
    bool result = true;
    
    std::vector<CameraEngine*>::iterator iter;
    for(iter = cameras_.begin(); iter != cameras_.end(); iter++)
    {
        if((*iter) == NULL)
        {
            printf("Multicam: child-cam could not be initialized.");
            return false;
        }
        
        result &= (*iter)->initCamera();
    }
    
    if(!cam_buffer)
        cam_buffer = new unsigned char[cfg->frame_height * cfg->frame_width * cfg->cam_format];
    
    return result;
}

bool MultiCamera::closeCamera()
{
    bool result = true;
    
    std::vector<CameraEngine*>::iterator iter;
    for(iter = cameras_.begin(); iter != cameras_.end(); iter++)
    {
        if((*iter) == NULL) continue;
        
        result &= (*iter)->closeCamera();
    }
    
    if(cam_buffer)
    {
        delete[] cam_buffer;
        cam_buffer = NULL;
    }
    
    return result;
}

bool MultiCamera::startCamera()
{
    bool result = true;
    
    std::vector<CameraEngine*>::iterator iter;
    for(iter = cameras_.begin(); iter != cameras_.end(); iter++)
        result &= (*iter)->startCamera();
    
    if(!result)
        stopCamera();
    
    running = result;
    
    return result;
}

bool MultiCamera::stopCamera()
{
    bool result = true;
    
    std::vector<CameraEngine*>::iterator iter;
    for(iter = cameras_.begin(); iter != cameras_.end(); iter++)
        result &= (*iter)->stopCamera();
    
    running = !result;
    
    return result;
}

bool MultiCamera::resetCamera()
{
    bool result = true;
    
    std::vector<CameraEngine*>::iterator iter;
    for(iter = cameras_.begin(); iter != cameras_.end(); iter++)
        result &= (*iter)->resetCamera();
    
    return result;
}

unsigned char* MultiCamera::getFrame()
{
    std::vector<CameraEngine*>::iterator iter;
    unsigned char* childcam_buffer_start = cam_buffer;
    int row_column = 0;
    for(iter = cameras_.begin(); iter != cameras_.end(); row_column++, iter++)
    {
        CameraEngine* cam = *iter;
        
        int line_size = cam->getWidth() * cam->getFormat();
        
        unsigned char* buffer_write = childcam_buffer_start;
        unsigned char* child_frame = cam->getFrame();
        if(child_frame == NULL) return NULL;
        
        for(int line_nr = 1; line_nr <= cam->getHeight(); line_nr++)
        {
            memcpy(buffer_write, child_frame, line_size);
            child_frame += line_size;
            buffer_write += line_size * cameras_columns_;
        }
        
        if(cameras_columns_ <= 1 || row_column == cameras_columns_ - 1)
        {
            row_column = 0;
            childcam_buffer_start = buffer_write;
        }
        else
            childcam_buffer_start += line_size;
            
    }
    
    return cam_buffer;
}

int MultiCamera::getCameraSettingStep(int mode) { return cameras_[0]->getCameraSettingStep(mode); }
int MultiCamera::getCameraSetting(int mode) { return cameras_[0]->getCameraSetting(mode); }
int MultiCamera::getMaxCameraSetting(int mode) { return cameras_[0]->getMaxCameraSetting(mode); }
int MultiCamera::getMinCameraSetting(int mode) { return cameras_[0]->getMinCameraSetting(mode); }
bool MultiCamera::getCameraSettingAuto(int mode) { return cameras_[0]->getCameraSettingAuto(mode); }
int MultiCamera::getDefaultCameraSetting(int mode) { return cameras_[0]->getDefaultCameraSetting(mode); }
bool MultiCamera::hasCameraSetting(int mode) { return cameras_[0]->hasCameraSetting(mode); }
bool MultiCamera::hasCameraSettingAuto(int mode) { return cameras_[0]->hasCameraSettingAuto(mode); }

bool MultiCamera::setCameraSettingAuto(int mode, bool flag) 
{
    bool result = true;
    std::vector<CameraEngine*>::iterator iter;
    for(iter = cameras_.begin(); iter != cameras_.end(); iter++)
        result &= (*iter)->setCameraSettingAuto(mode, flag);
    return result;
}

bool MultiCamera::setCameraSetting(int mode, int value)
{ 
    bool result = true;
    std::vector<CameraEngine*>::iterator iter;
    for(iter = cameras_.begin(); iter != cameras_.end(); iter++)
        result &= (*iter)->setCameraSetting(mode, value);
    return result;
}

bool MultiCamera::setDefaultCameraSetting(int mode)
{
    bool result = true;
    std::vector<CameraEngine*>::iterator iter;
    for(iter = cameras_.begin(); iter != cameras_.end(); iter++)
        result &= (*iter)->setDefaultCameraSetting(mode);
    return result;
}