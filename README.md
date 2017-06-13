# reacTIVision 1.6

© 2005-2016 by Martin Kaltenbrunner <martin@tuio.org>
[https://github.com/mkalten/reacTIVision](https://github.com/mkalten/reacTIVision)

**reacTIVision** is an open source, cross-platform computer vision framework for the fast and robust tracking of fiducial markers attached onto physical objects, as well as for multi-touch finger tracking. It was mainly designed as a toolkit for the rapid development of table-based tangible user interfaces (TUI) and multi-touch interactive surfaces. This framework has been developed by Martin Kaltenbrunner and Ross Bencina as part of the [Reactable project](http://www.reactable.com/), a tangible modular synthesizer.


**reacTIVision** is a standalone application, which sends `Open Sound Control` (OSC) messages via a `UDP network socket` to any connected client application. It implements the [TUIO](http://www.tuio.org/) protocol, which has been specially designed for transmitting the state of tangible objects and multi-touch events from a tabletop surface.

The [TUIO](http://www.tuio.org/) framework includes a set of example client projects for various programming languages, which serve as a base for the easy development of tangible user interface or multi-touch applications. Example projects are available for languages such as:
- C/C++
- C#
- Java
- [Processing](https://processing.org/)
- [Pure Data](https://puredata.info/)
- Max/MSP
- Flash

The **reacTIVision** application currently runs under the following operating systems: Windows, Mac OS X and Linux.

Under **Windows** it supports any camera with a proper `WDM driver`, such as most USB, FireWire and DV cameras. Under **Mac OS X** most UVC compliant USB as well as Firewire cameras should work. Under **Linux** FireWire cameras are as well supported as many `Video4Linux2` compliant USB cameras.

---
1. [Fiducial Symbols](#fiducial-symbols)
2. [Finger Tracking](#finger-tracking)
3. [Blob Tracking](#blob-tracking)
4. [Application Handling](#application-handling)
5. [XML configuration file](#xml-configuration-file)
6. [Calibration and Distortion](#calibration-and-distortion)
7. [Application Cheatsheet](#application-cheatsheet)
8. [Compilation](#compilation)


## Fiducial Symbols

This application was designed to track specially designed fiducial markers. You will find the default *amoeba* set of 216 fiducials in the document `./symbols/default.pdf`. Print this document and attach the labels to any object you want to track.

**reacTIVision** detects the ID, position and rotation angle of fiducial markers in the realtime video and transmits these values to the client application via the [TUIO](http://www.tuio.org/) protocol. [TUIO](http://www.tuio.org/) also assigns a session ID to each object in the scene and transmits this session ID along with the actual fiducial ID. This allows the identification and tracking of several objects with the same marker ID.

## Finger Tracking

**reacTIVision** also allows multi-touch finger tracking, which is basically interpreting any round white region of a given size as a finger that is touching the surface. Finger tracking is turned off by default and can be enabled by pressing the `F` key and adjusting the average finger size, which is given in pixels. The general recognition sensitivity can also be adjusted, where its value is given as a percentage. 75 would be less sensitive and 125 more sensitive, which means that also less probable regions are interpreted as a finger. The finger tracking should work with DI (diffuse illumination) as well as with FTIR illumination setups.

## Blob Tracking

Since version 1.6 **reacTIVision** also can track untagged objects, sending the overall footprint size and orientation via the [TUIO](http://www.tuio.org/) blob profile.
You can activate this feature and configure the maximum blob size in `./reacTIVision.xml`, where you can also choose to send optional blob messages for fingers and fiducials, in order to receive information about their overall size. These blobs will share the same session ID with the respective finger or fiducial to allow the proper assignment.

## Application Handling
[usage.md](usage.md)


## Compilation
[compilation.md](compilation.md)


## License

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
