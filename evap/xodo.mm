
xodometer

	Track the total distance your pointing device and cursor
	travel.  The distance can be displayed in various units.

	xodometer displays total distance and "trip" distance
	since the application started (or since you clicked on
	a trip reset button).

	xodometer requires certain information to ensure
	accurate distance tracking.  Refer to the following
	sections to learn about calibrating xodometer, and to
	view a list of tested configurations.

	xodometer is typically started from the .xinitrc file.
	Use the left button for selections.  Use the middle
	button to reset both trip odometers simultaneously.

	Every xodometer command line parameter can have an
	application resource in the xrdb database or .Xdefaults
	file.  A resource follows this convention:

	  xodo.parameter_name : parameter_value

	Therefore to specify xodometer's default font the
	following resource entry could be specified:

	  xodo.fontname : Rom8

	For further help try xodo -full_help.

	Examples:

	  xodo -bd red -bg wheat1 -fg blue -g -0-0

	  xodo -fn rom6 -o cursor -dwm 300 -dhm 234 


	CALIBRATION

	xodometer requires the display dimensions in both pixels
	and millimeters in order to correctly compute distances.
	Look for this information in the appropriate hardware 
	reference manual for your display.  If you cannot find
	this information it's still easy to determine.  For the
	display dimensions in pixels simply run xodometer, jam
	the cursor in the bottom-right corner and note the X/Y
	coordinates displayed at the bottom of the window - add
	one to get the actual pixel count.  For the display
	dimensions in millimeters just grab a tape measure and
	measure your screen - if inches multiply by 25.4 and if
	centimeters multiply by 10.  Select "About" and	verify
	that xodometer is calibrated properly by using a ruler
	to measure the calibration scale.  The default values
	for these dimensions are suitable for an IBM RS/6000
	machine with a 6091 19" color monitor.  Refer to the
	next section for values of other tested	configurations.

	Assuming that the display dimension data is correct the
	cursor distance can be accurately tracked.  The actual
	distance that your pointing device, typically a mouse,
	travels is INFERRED by accleration information provided
	by the X server and pointer scaling information that
	you must provide.  The default scale factor is 3.4,
	meaning that the cursor travels 3.4 times as far as the
	pointing device moves. This value is appropriate for an
	IBM RS/6000 machine with a 6091 19" color monitor.
	Refer to the next section for values of other tested
	configurations.

	If you cannot find the correct scale factor for your
	mouse then you must determine it by measuring.  It is
	rather easy to do this:  first enter "xset m 1 1" to
	set the X threshold and	acceleration to 1, then enter
	"xodo -psf 1.0" to set xodometer's pointer scale factor
	also to 1.  Once xodo is running pull-down the Units
	menu and select "inches".  Then, using a ruler, place
	the pointing device against one edge, click the second
	button to reset the trip odometers, and then trace a
	known distance, say, one inch.  The distance recorded
	by the pointer's trip odometer is the proper scaling
	factor.  Repeat the measurement several times for
	accuracy.

	At the bottom of the xodometer window is a status line
	that displays the current distance Units and the X/Y
	cordinates of the cursor.  In the "About" window the
	pointer	Scale factor, and the X Threshold and
	Acceleration are displayed.

	All the calibration information you supply is either
	passed on the command line, stored in environment
	variables, or placed in your .Xdefaults file.  The
	applicable environment variables are:
	
	  D_XODO_DWM	display_width_millimeters
	  D_XODO_DHM	display_height_millimeters
	  D_XODO_DWP	display_width_pixels
	  D_XODO_DHP	display_height_pixels
	  D_XODO_PSF	pointer_scale_factor


        TESTED CONFIGURATIONS	

	For each machine, Operating System/window manager and
	display configuration, a sample xodometer command line
	is given:

	- Pentium-100, Linux 1.2.13/fvmw, 15" color
	  xodo -dwm 286 -dhm 203 -dwp 1024 -dhp 768  -psf 3.0

	- IBM RS/6000, AIX 3.2.3/mwm, 16" color
	  xodo -dwm 300 -dhm 234 -dwp 1280 -dhp 1024 -psf 3.0

	- IBM RS/6000, AIX 3.2.3/mwm, 19" color
	  xodo -dwm 350 -dhm 274 -dwp 1280 -dhp 1024 -psf 3.4

	- IBM RS/6000, AIX 3.2.3/mwm, 23" color
	  xodo -dwm 430 -dhm 340 -dwp 1280 -dhp 1024 -psf 4.0

	- Sun SPARC 1+, SunOS 4.1.1/twm, 17" monochrome
	  xodo -dwm 292 -dhm 232 -dwp 1152 -dhp  900 -psf 2.0	  

	- Sun SPARC 1+, SunOS 4.1.1/twm, 19" color
	  xodo -dwm 358 -dhm 274 -dwp 1152 -dhp  900 -psf 4.0	  
.display
	The X display name; default is the DISPLAY variable.
.display_width_millimeters
	The width in millimeters of the X display.  The default
	is 350 mm (an IBM 6091 19" color monitor).
.display_height_millimeters
	The height in millimeters of the X display.  The default
	is 274 mm (an IBM 6091 19" color monitor).
.display_width_pixels
	The width of the X display in pixels.  The default
	is 1280 pixels (an IBM 6091 19" color monitor).
.display_height_pixels
	The height of the X display in pixels.  The default
	is 1024 pixels (an IBM 6091 19" color monitor).
.pointer_scale_factor
	The scale factor to convert pointer movement to cursor
	movement.  A scale factor of 2.0 means that for every D
	units of distance the pointing device moves, the cursor
	moves 2 * D units.  The default is 3.4, suitable for an
	IBM 6091 19" color monitor.
.border
	xodometer's border color.
.background
	xodometer's background color.
.foreground
	xodometer's foreground color.
.fontname
	xodometer's odometer font.  An extremely small font
	is "rom6" while a rather large font is "helvr30".
.fontname2
	xodometer's button font.  In general you should NOT
	change this font since the action buttons do not
	change size.  Another suitable font is "Rom8", but
	that is not available on all X servers.
.geometry
	Specifies the X geometry in the standard notation.
	The width and height are not normally specified since
	xodometer calculates them based on the fontname.  If
	an "offset" value is positive it is measured from the
	top or left edge of the display, and if negative it is
	measured from the bottom or right edge of the screen.
	So, to start xodometer in the bottom-right corner a
	geometry string of "-0-0" would be specified.
.iconic
	If specified xodometer starts up already iconified.
.microsecond_interval_time
	The number of microseconds between odometer updates.  The
	default value of 100,000 means that the pointer position
	is sampled 10 times per second, which seems to provide
	accurate distance measurements without consuming
	excessive amounts of your machine's resources.
.odometer
	A keyword that specifies whether to display both
	odometers, or just one of them, and if just one,
	which one.
.odometer_file
	The path name of the file to record total mouse distance
	(in millimeters) and other application information.  This
	file is read during xodometer startup to initialize the
	distance totals and establish the distance units.  When
	you "Quit" xodometer the updated distance/unit data is
	written to this file.
.odometer_autosave_time
	Specifies the time interval in minutes between odometer
	file updates.  This is just for good luck, as xodometer
	updates the odometer file when these event are received:

	  - control/c
	  - window close
	  - window manager exit
.title
	The xodometer window title line.
