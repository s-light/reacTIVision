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

#ifndef FIDUCIALFINDER
#define FIDUCIALFINDER

#include <vector>
#include <list>
#include <sstream>
#include <string>

#include "FrameProcessor.h"
#include "TuioServer.h"
#include "TuioObject.h"
#include "TuioCursor.h"
#include "TuioManager.h"

#include "floatpoint.h"

#ifdef __APPLE__
#include <CoreFoundation/CFBundle.h>
#endif

#define FINGER_ID -1
#define BLOB_ID -2

class FiducialFinder: public FrameProcessor
{
public:
	FiducialFinder(TUIO::TuioManager *manager) {
		
		this->tuioManager = manager;
		
		calibration = false;
		totalframes = 0;
		session_id = 0;
		
		currentSetting = INV_NONE;
		show_settings = false;

		dmap = NULL;
	}
	
	~FiducialFinder() {
		if (dmap) delete[] dmap;
		//delete xposfil,yposfil,anglefil,widthfil,heightfil;
	}
	
	virtual void process(unsigned char *src, unsigned char *dest) = 0;
	bool init(int w, int h, int sb ,int db);
	bool toggleFlag(unsigned char flag, bool lock);
    void computeGrid();
	
protected:
	int session_id;
	long totalframes;
	
	const char* grid_config;
	int cell_width, cell_height;
	int grid_size_x, grid_size_y;
	
	bool calibration, show_grid, empty_grid;
	ShortPoint* dmap;
	
	void displayControl();
	void drawObject(int id, float xpos, float ypos, int state);
	void drawGrid(unsigned char *src, unsigned char *dest);
	
	TUIO::TuioManager *tuioManager;

	DisplayMode prevMode;
	bool show_settings;
	
private:
	enum InvertSetting { INV_NONE, INV_XPOS, INV_YPOS, INV_ANGLE };
	int currentSetting;
	
};

#endif
