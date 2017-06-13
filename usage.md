## Application Handling

Before starting the **reacTIVision** application make sure you have a supported camera connected to your system, since the application obviously will not work at all without a camera. The application will show a single window with the B&W camera video in realtime.

Pressing the `S` key will show to the original camera image. Pressing `T` will show the binary tresholded image, pressing the `N` key will turn the display off, which reduces the CPU usage.

The thresholder gradient gate can be adjusted by hitting the`G` key. Lowering the value can improve the thresholder performance in low light conditions with insufficient finger contrast for example. You can gradually lower the value before noise appears in the image. Optionally you can also adjust the tile size of the thresholder.

The camera options can be adjusted by pressing the `O` key. On Windows and Mac OS this will show a system dialog that allows the adjustment of the available camera parameters. On Linux (Mac OS X when using Firewire cameras), the available camera settings can be adjusted with a simple on screen display. Pressing the `K` key allows
to browse and select all available cameras and image formats.

In order to produce some more verbose debugging output, hitting the `V` will print the symbol and finger data to the console. Pressing the `H` key will display all these options on the screen. `F1` will toggle the full screen mode, the `P` key pauses the image analysis completely, hitting `ESC` will quit the application.

## XML configuration file

**Common settings** can be edited within the file `./reacTIVision.xml` where all changes are stored automatically when closing the application.

Under Mac OS X this XML configuration file can be found within the application bundle's Resources folder. Select *Show Package Contents"* from the application's context menu in order to access and edit the file.

The **reacTIVision** application usually sends `TUIO/UDP` messages to port `3333` on `localhost (127.0.0.1)`. Alternative transport types are
- `TUIO/TCP` (TCP)
- `TUIO/WEB` (Websocket)
- `TUIO/FLC` (Flash Local Connection).

You can change this setting by editing or adding the XML tag `<tuio type="udp" host="127.0.0.1" port="3333">` to the configuration. Multiple (up to 32) simultaneous TUIO transport options are allowed, the according type options are: udp, tcp, web and flc.

The `<fiducial amoeba="default" yamaarashi="false" />` XML tag lets you select the default or alternative (small,tiny) *amoeba* fiducial set, or you can point to an alternative `amoeba.trees` file if available. You can also additionally enable the new *Yamaarashi* fiducal symbols.

The **display attribute** defines the default screen upon startup. The `<image display="dest" equalize="false" gradient="32" tile="10"/>` lets you adjust the default gradient gate value and tile size. **reacTIVision** comes with an image equalization module,  which in some cases can increase the recognition performance of both the finger and fiducial tracking. Within the running application you can toggle this with the `E` key or reset the equalizer by hitting the `SPACE` bar.

The overall **camera and image settings** can be configured within the `./camera.xml` configuration file. On Mac OS X this file is located in the Resources folder within the application bundle. You can select the camera ID and specify its dimension and framerate, as well as the most relevant image adjustments. Optionally you can also crop the raw camera frames to reduce the final image size.

*Please see the example options in the file for further information.*

You can list **all available cameras** with the `-l` startup option.

## Calibration and Distortion

Many tables, such as the **reacTable** are using wide-angle lenses to increase the area visible to the camera at a minimal distance. Since these lenses unfortunately distort the image, **reacTIVision** can correct this distortion as well as the alignment of the image.

For the **calibration**, print and place the provided calibration sheet on the table and adjust the grid points to the grid on the sheet. To calibrate **reacTIVision** switch to calibration mode hitting `C`.

Use the keys `A`,`D`,`W`,`X` to move within grid, moving with the cursor keys will adjust the position (center point) and size (lateral points) of the grid alignment. By pressing `Q` you can toggle into the precise calibration mode, which allows you to adjust each individual grid point.

`J` resets the whole calibration grid, `U` resets the selected point and `L` reverts to the saved grid.

To check if the distortion is working properly press `R`. This will show the fully distorted live video image in the target window. Of course the distortion algorithm only corrects the found fiducial positions instead of the full image.

##### Application Cheatsheet

| Key 		| Function 										|
| ---------	| --------------------------------------------- |
| `G` 		| Threshold gradient & tile size configuration	|
| `F`		| Finger size and sensitivity configuration		|
| `B`		| Blob size configuration & enable /tuio/2Dblb	|
| `Y` 		| enable/disable Yamaarashi symbol detection	|
| `I` 		| configure TUIO attribute inversion			|
| `E`		| Turns image equalization on/off				|
| `SPACE` 	| Resets image equalization 					|
|			|												|
| `O` 		| Camera configuration							|
| `K` 		| Camera selection								|
|			|												|
| `C` 		| Enter/Exit Calibration mode					|
| `Q` 		| Toggle quick/precise calibration mode 		|
| `U` 		| Resets selected calibration grid point		|
| `J` 		| Resets all calibration grid points			|
| `L` 		| Reverts calibration to saved grid				|
|			|												|
| `S`		| Shows original camera image 					|
| `T` 		| Shows binary thresholded image				|
| `R` 		| Shows calibrated camera image					|
| `N` 		| Turns display off	(saves CPU)					|
| `F1`		| Toggles full screen mode 						|
|			|												|
| `P` 		| Pauses all processing							|
| `V` 		| Verbose output to console						|
| `H` 		| Shows help options							|
| `ESC` 	| Quits application 							|
