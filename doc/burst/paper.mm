.\" ASI Note: PP was changed to P in this file; CS was commented out
.PH ""
.PF "''\\\\nP''"
.so defs.mm
.so abstract.mm
.so macros.mm
.H 1 "User Interface"
.H 2 "Program Invocation"
.P
In the examples below,
the `$' symbol represents the \*(U)
shell prompt.
The general usage for the \*(b) program is as follows:
.Ex "burst [-b]\fR"
.P
When \*(b) is used in the default mode,
a full-screen, interactive, menu-driven interface is provided:
.Ex burst
In the interactive mode,
commands are formatted by the menu system (see \fBHierarchical Menus\fR).
The optional
.I -b\^
argument will suppress the full-screen terminal display in favor of a
batch-oriented interface;
when coupled with \*(U) file redirection and background processing,
the user can submit the job and go on to other tasks.
.Ex "burst \|\-b \|< \|input_file \|&"
The user will typically want to redirect any diagnostics to a log file
(see \fBError Log\fR).
In either batch or interactive mode,
the program will read commands (see \fBCommand Input\fR) from the standard
input until an end of file is encountered.
In batch mode,
the program will exit after the last command is processed,
but when run interactively the menu system will take over after the last
command.
The user must \fIquit\fR the program by exiting the top-level menu
(see Table 2).
It is recommended that the reader begin by running the \*(b)
program interactively.
The hierarchical menu system generates key word commands for the user by
prompting him to make choices or enter values without prior knowledge of
the commands and their syntax.
As commands are generated,
they are printed in the scrolling region so that the user can learn
the syntax of the alternative mode of input without affecting his productivity.
The more advanced user can circumvent the menu system by using a
text editor to prepare input files.
.sp
.H 2 "Command Input"
.P
All commands to \*(b) are composed of one or more words connected by hyphens
and each command may require one or more arguments that must be separated by
either spaces or tabs.
A line that begins with the `#' symbol is considered a comment and will be
ignored.
Each command must be on a separate line and may be supplied to the \*(b)
program's standard input (i.e. using \*(U)
.Ud sh 1
file redirection) or can be generated from the hierarchical menus.
In Table 1 below is a brief description of these commands; \fIflag\fR
arguments have \fIyes\fR or \fIno\fR values, an \fIangle\fR is expressed in
degrees as a floating-point quantity,
\fIdistances\fR and coordinates such as \fIX, Y, Z, left, right\fR, etc.
are also floating-point numbers,
and \fIcount\fR represents an integer.
Square brackets delimit optional arguments.
For a detailed description of each command,
refer to sections 2 through 7.
.DF
.sp
.TB "Key Word Commands"
.so commands.mm
.sp
.DE
.H 2 "Screen Layout"
.P
In interactive mode,
the screen is divided into sections of one or more rows (lines) of text.
.DF
.so screen.mm
.FG "Screen Layout"
.sp
.DE
The top line,
referred to as line 1,
is reserved for
.SM CPU
usage statistics.
Starting on line 2,
the hierarchical menu region spans 12 lines of the screen.
Below the menu region is a line used for user input related errors and
help messages that are generated by typing an `h' (see Table 2).
Below that is a line used to prompt the user for input and to inform him
of ongoing processing.
The next line contains the program name and version number,
and is intended as a divider between the scrolling window below it and the
non-scrolling portion of the screen.
Once ray tracing has begun,
grid index ranges will appear inside square brackets to the right of the
version number.
The first number is the current horizontal index,
and this is separated by a colon from the right-most index for the grid.
After the comma,
is the current vertical index,
followed by the top-most index for the grid.
As ray tracing commences,
the current grid indices will reflect the current shotline being calculated
as the grid is traversed in a left-to-right,
bottom-to-top fashion.
The scrolling window region takes up the remainder of the screen for the
display of processed commands and processing-related errors (i.e. from the
ray tracer).
Figure 1 shows what the various sections of the screen are used for,
however in practice,
the status information,
help message,
and prompt would not be displayed concurrently.
The screen-oriented interface is terminal independent;
it doesn't use graphics per se
but uses the terminal capabilities information database to control the
manipulation of text characters.
.sp
.H 2 "Hierarchical Menus"
.P
Table 2 shows the key strokes used to manipulate the menus.
.DF
.sp
.TB "Hierarchical Menu Commands"
.so hmkeys.mm
.sp
.DE
The top-level menu looks like Figure 2.
.DF
.so topmenu.mm
.FG "Top-Level Hierarchical Menu"
.sp
.DE
Note that the \fIcurrent\fR entry will be highlighted.
The `v' symbols at the bottom corners of the menu show that
there are more entries below.
Attempting to move the cursor past the bottom entry will cause the
menu to scroll upwards as illustrated in Figure 3.
.DF
.so scrollmenu.mm
.FG "Scrolled Menu"
.sp
.DE
Note the `^' symbols at the top corners of the menu.
These signify that there are now hidden entries above that can be
scrolled down by advancing the cursor upwards past the top visible
entry.
If no scrolling is possible,
that is,
if no further entries exist,
a `+' symbol will mark the corner of the menu.
If a menu item is selected that has a sub-menu,
the sub-menu will be drawn to the right of its parent menu as shown
in Figure 4.
.DF
.so submenu.mm
.FG "Sub-Menu Display"
.sp
.DE
The menu system \fIremembers\fR the last item selected in a given
menu;
entering a menu that has been used before will cause
the \fIcurrent\fR item to be the same as it was when that menu was last exited.
If a menu has never been used,
the top item will initially be \fIcurrent\fR.
.H 2 "Scrolling Region"
.P
When running the \*(b) program interactively,
all diagnostic output is, by default, printed in a scrolling text window
at the bottom of the display.
If the batch option (\fI\-b\fR) is used,
these diagnostics will be printed on the standard error output of the
program.
If there is enough output the user may find it difficult
to read it all before it scrolls out of sight.
Also, the user may wish to save these diagnostics for later reference;
overlap diagnostics from the ray tracer are useful in fixing errors in
the geometry of the target description.
The \*(b) program has a command called
.Co error-file
that will divert this output to a file (see \fBError Log\fR).
Figure 5 shows three such overlap diagnostics.
.DS
.sp
.so overlap.mm
.FG "Overlap Diagnostics"
.sp
.DE
The full path name of both regions is printed, the zero enclosed in double
curly brackets is intended to discriminate between \fIinstances\fR\*F.
.FS
An \fIinstance\fR is a type of group in the \*(m) data base that has
a \fIprototype\fR object as a member.
Several \fIinstances\fR may reference the same \fIprototype\fR
yet have different transformation matrices.
Although these objects represent distinct geometric entities,
they share the same path name,
therefore the additional discriminant is necessary.
.FE
Theoretically,
\fIisol\fR and \fIosol\fR are the names of the starting and ending solids
associated with the boolean operations on the overlapping partition.
In practice,
these solid names are typically not helpful in diagnosing the problem,
but the region names should be sufficient.
The \fIdepth\fR is the line-of-sight thickness of the overlapping partition
in millimeters.
In parentheses,
are printed the target coordinates of the intersection of the ray
with the overlap.
The \fIx-2\fR and \fIy1\fR reveal that the grid indices of the shotline are
\fI-2, 1\fR; this means that the shotline was 2 cells to the left,
and one cell above the grid origin.
If \fIlvl\fR (meaning ray tracing recursion level) is \fIzero\fR,
then the overlap resulted from a shotline,
but if it is \fIone\fR,
it could represent either a burst ray intersection or a probe to
calculate the normal thickness of a component intersected by the shotline.
The real purpose of the ray is stated last.
.H 1 "Executing a Run"
.Cs "/execute"
The \*(b) program is designed to allow many options to be configured before
any outputs are calculated.
Unless otherwise stated in the individual command descriptions in this report,
a given command will nullify any previous occurrence of the same directive
until an
.Co execute
command is given.
For instance, the command
.Co "target-file"
\ \fItank1\fR followed by
.Co "target-file"
\ \fItank2\fR will nullify the reference to \fItank1\fR.
Once the input parameters are specified,
the
.Co execute
command will start a \fIrun\fR.
Many \fIruns\fR may occur during an instance of the \*(b) program.
One such instance is refered to as a \fIsession\fR.
Below are descriptions of commands for configuring the behavior of the
\*(b) program.
.Ce
.H 1 "User Preferences"
.H 2 "Units of Measure"
.Cs /units
The
.Co units
command will set the linear units for input and output.
This command should be used before any scalar quantities such as coordinates,
distances,
or sizes are input.
The units may be changed to accommodate input files of differing units,
but the output from a particular run will reflect whatever the units were
set to when the
.Co execute
command was given.
One argument is expected out of the following list and must be spelled
correctly:
\fImillimeters\fR,
\fIcentimeters\fR,
\fImeters\fR,
\fIinches\fR and \fIfeet\fR.
The default units are \fImillimeters\fR.
Angles are always expressed in degrees, not radians.
.Ce
.H 2 "Region Overlap Reporting"
.Cs "/preferences/report-overlaps"
It is considered an error if two \fIregions\fR in an \*(m) file occupy
the same space; we call this an \fIoverlap\fR.
The ray tracing library (\*(l)) will report \fIoverlapping regions\fR that are
intersected by shotlines or burst rays to the \*(b) application and
the program will,
by default,
print out any that have a line of sight thickness of at least 0.25 millimeters
(see \fBError Log\fR).
Although a target may only have several \fIoverlaps\fR,
an error will be reported for each ray that intersects one of them,
so the messages will typically be repetitious.
Although these diagnostics are important for fixing problems in the geometric
description of the target,
the user may wish to proceed with a production run and the printing of these
errors can slow the execution time considerably.
A \fIyes\fR or \fIno\fR argument to this command will turn the diagnostics
on or off.
Regardless of whether or not individual \fIoverlaps\fR are reported,
the total number detected will be logged.
.Ce
.H 1 "Shotlining Options"
.Cs /shotlines
\fIShotlining\fR is a technique whereby lines are described in the target
coordinate system and information is requested about the geometry that
intersects those lines in 3-space.
This technique is useful for analysis programs that must simulate
threat/target interactions,
and therefore must sample the geometry along the threat path.
Typically the lines are specified discretely,
by a point and a direction,
or a \fIgrid\fR of lines is generated that is
oriented perpendicular to the direction of attack.
A \fIgrid\fR is rectangular,
but is subdivided along its height and width uniformly resulting in square
cells.
\fIGridding\fR techniques include passing a line called a \fIshotline\fR through
the center of each cell,
or alternatively,
\fIdithering\fR each shotline's position within its respective cell's
boundaries (see \fBDithering Shotlines\fR).
.Ce
.H 2 "Attack Direction"
.Cs "/shotlines/attack-direction"
The orientation of shotlines with respect to the coordinate system of the
target are described by \fIazimuth\fR and \fIelevation\fR angles.
These angles must be specified in degrees as floating-point numbers
(see \fBGridding\fR).
.Ce
.H 2 "Gridding"
.Cs "/shotlines/shotline location"
The user is faced with several choices for generating shotlines;
full-target envelope, partial envelopes, or discrete shots.
No matter what shotlining method is used,
a grid always exists as a frame of reference for specifying 2-dimensional
coordinates in the plane normal to the direction of attack.
This 2-dimensional coordinate system is a projection of the shotline
coordinate system (also refered to as the \fIprimed\fR coordinate system).
For the simple case of a zero azimuth, zero elevation attack,
the
.Ax \*(x) \*(y) \*(z)
in the shotline coordinate system coincide
with the
.Ax \*(X) \*(Y) \*(Z)
of the target coordinate system and the shotline direction is parallel to
the \*(x)\*(A) and headed toward decreasing coordinates.
Other orientations are described by rotating the
.Ax \*(x) \*(y) \*(z)
to keep the shotline direction always down the \*(x)\*(A).
This transformation involves two rotations;
first a rotation of the \fIprimed\fR coordinate system about the coincident
\*(Z) and \*(z)\*(a) by the specified azimuthal angle,
followed by a rotation about the new \*(y)\*(A) by the specified elevation
angle.
Since the grid is a 2-d projection of the shotline coordinate system,
it has no \*(X) coordinate;
if the user's viewpoint is from the direction of attack,
the \*(y)\*(A) can be thought of as horizontal with increasing coordinates
to the right,
and the \*(z)\*(A) as vertical and increasing in the upward direction.
.Ce
.Cs "/shotlines/shotline location/envelope"
An \fIenvelope\fR refers to a grid that is dimensioned such that its
rectangular area,
projected normal to the grid,
will cover optionally all or part of the target.
.Ce
.Cs "/shotlines/shotline location/envelope/enclose-target"
This option will generate a grid that is guaranteed to cover the entire
target.
Since the \*(m) modeler uses combinatorial solid geometry,
the dimensions of the target are not known,
therefore a worst case bounding rectangular parallel piped (RPP)
is the only information available to the program and
the grid may be larger than necessary.
In addition,
depending on the attack aspect,
the presented area of some targets may not fill up a rectangular grid well.
This should not be a problem since ray tracing outside the target
boundaries is cheap, but if desired,
the grid can be trimmed down with the partial envelope option described
below.
The grid origin is always aligned with the target origin.
.Ce
.Cs "/shotlines/shotline location/envelope/enclose-portion"
This option allows the user to generate a \fIsub-grid\fR by specifying the
distances from the grid origin to the \fIsub-grid's\fR left, right, top,
and bottom boundaries.
.Ce
.H 2 "Cell Size"
.Cs "/shotlines/cell-size"
The dimensions of a grid cell are input as floating-point values that
represent the distances between the centers of adjacent cells.
\fICell size\fR also expresses the projected area of influence
associated with a shotline or burst ray.
Therefore,
\fIcell size\fR must be specified even when a grid will not be generated,
such as with discrete shot or discrete burst point selection
(see \fBInput Discrete Shots\fR and \fBInput Discrete Burst Points\fR).
.Ce
.H 2 "Dithering Shotlines"
.Cs "/shotlines/dither-cells"
When gridding,
shotlines normally pass through the center of each cell,
however,
they may be also be \fIdithered\fR.
If the user chooses the latter,
2 random numbers are selected for each
cell that are used to offset the shotline in both parametric
directions of the grid plane,
but within the respective cell's boundaries.
.Ce
.H 2 "Input Discrete Shots"
.Cs "/shotlines/shotline location/discrete shots"
The following options describe different ways to input explicit shot locations
as an alternative to generating a grid.
.Ce
.Cs "/shotlines/shotline location/discrete shots/target coordinate system"
If the user wants to fire at a known point on the target,
he may wish to describe the shotline location in target coordinates.
When coupled with the attack direction,
each 3-dimensional coordinate uniquely specifies a shotline.
.Ce
.Cs "/shotlines/shotline location/discrete shots/target coordinate system/input-3d-shot"
This option allows the user to type in a single shot location as an
\*(X), \*(Y), and \*(Z) coordinate.
When the
.Co execute
command is given,
that one shot will be run.
No queueing of shots occurs in this mode;
the last set of coordinates entered will be used.
.Ce
.Cs "/shotlines/shotline location/discrete shots/target coordinate system/read-3d-shot-file"
A number of shots can be input from a file;
this option will,
after submission of an
.Co execute
directive,
loop through every set of target coordinates in the named file.
The file should contain three floating-point numbers on each line separated by
white space (blanks or tabs).
.Ce
.Cs "/shotlines/shotline location/discrete shots/shotline coordinate system"
Another way to describe a shot location is in the shotline coordinate
system.
Since the \*(x) location of the shot is irrelevant (the shotline is
parallel to the \*(x)\*(A)) a shot may be specified as a
\*(y) and \*(z) coordinate.
These coordinates can also be refered to as horizontal and vertical grid
offsets.
.Ce
.Cs "/shotlines/shotline location/discrete shots/shotline coordinate system/input-2d-shot"
This option allows the user to type in a single shot location as a
\*(y), and \*(z) coordinate.
When the
.Co execute
command is given,
that one shot will be run.
No queueing of shots occurs in this mode,
the last set of grid offsets entered will be used.
.Ce
.Cs "/shotlines/shotline location/discrete shots/shotline coordinate system/read-2d-shot-file"
A number of shots can be input from a file;
this option will,
after submission of an
.Co execute
directive,
loop through every set of grid offsets in the named file.
The file should contain three floating-point numbers on each line separated by
white space (blanks or tabs).
.Ce
.H 1 "Bursting Options"
.Cs "/burst points"
\fIBursting\fR is a technique for sampling a target's geometry with the use
of ray tracing.
As opposed to \fIshotlining\fR involving parallel rays,
\fIbursting\fR employs a distribution of rays that emanate from a single
point.
The \*(b) program generates rays that approximate a uniform distribution
over a user-specified solid angle (see \fBSampling Cone Half Angle\fR)
and having a density (see \fBNumber of Sampling Rays\fR) that is also under
control of the user.
The user also has a choice between several mechanisms for setting up
burst point locations depending on the particular threat he is attempting
to emulate.
.Ce
.H 2 "Method of Locating Burst Point"
.Cs "/burst points/bursting method"
Depending on threat type,
burst points may be located using two basic techniques.
The first technique is simply to input the burst point coordinates.
This method can be used to compare vulnerability analysis results with
empirical results from the firing range or combat field.
The second technique available to the user is to burst along a shotline.
This option is used more for predicting the burst point location based
on target geometry,
given certain parameters that describe the target/threat interactions.
.Ce
.H 3 "Input Discrete Burst Points"
.Cs "/burst points/bursting method/burst point coordinates"
The input of explicit burst point coordinates can be accomplished
either by typing them in one at a time or by reading a file
of target \*(X), \*(Y), and \*(Z) coordinates.
.Ce
.Cs "/burst points/bursting method/burst point coordinates/burst-coordinates"
This option allows the user to type in a single burst point location as an
\*(X), \*(Y), and \*(Z) coordinate.
When the
.Co execute
command is given,
that one burst point will be run.
No queueing of burst points occurs in this mode,
the last set of coordinates entered will be used.
.Ce
.Cs "/burst points/bursting method/burst point coordinates/read-burst-file"
A number of burst points can be input from a file;
this option will,
after submission of an
.Co execute
directive,
loop through every set of target coordinates in the named file.
The file should contain three numbers on each line separated by
white space (blanks or tabs).
.Ce
.H 3 "Burst on Contact"
.Cs "/burst points/bursting method/burst point coordinates/shotline-burst"
The
.Co shotline-burst
directive can be given a \fIyes\fR or \fIno\fR argument to either enable
or disable this method of generating burst points.
When a \fIyes\fR argument is given,
a second \fIyes\fR or \fIno\fR argument is also required
(see \fBBurst on Armor\fR).
Bursting along a shotline can be done different ways depending on the
combination of several options.
The location of the burst point is based on the triggering
mechanism that is selected with the \fIburst distance\fR parameter.
.Ce
.H 4 "Burst on Armor"
.sp
If the \fIburst distance\fR parameter is set to a negative or zero value,
then interior burst points will be generated (see \fBBurst Distance\fR).
This method of bursting requires the input of \fIburst armor\fR idents and,
by default, \fIburst air\fR idents are also required.
If the user does not want to require that certain air be present to trigger
a burst point,
the
.Co shotline-burst
command has a second argument.
When this second argument is set to \fIno\fR,
bursting will occur as long as
\fIburst armor\fR is followed by any air or void (empty space),
and the \fIburst air\fR file is not required.
For more information see \fBBurst Armor File\fR and \fBBurst Air File\fR.
.Ce
.H 4 "Ground Plane Bursting"
.Cs "/burst points/bursting method/burst point coordinates/ground-plane"
Ground plane bursting is a vehicle for evaluating the effect of
fragmenting warheads on light-armored vehicles when they strike the ground
in close proximity to the target.
The
.Co ground-plane
command is only relevant when bursting along a shotline is selected.
The ground is modeled as a rectangle lying in a plane parallel
to the target \fIX-Y\fR plane with edges parallel to the \*(X) and \*(Y)
axes.
The grid will be enlarged to include the ground plane;
it is important for efficiency to limit the size of the ground plane to
match the range of the fragments that may be generated by the particular
threat being modeled.
When enabling this option,
the
.Co ground-plane
command is given a \fIyes\fR argument
followed by the height of the target above the ground,
and the distances that the ground rectangle extends out positive \*(X),
negative \*(X),
positive \*(Y),
and negative \*(Y) axes.
.Ce
.H 2 "Bursting Parameters"
.Cs "/burst points/bursting parameters"
The following parameters influence both the triggering mechanism for burst
points,
as well as the characteristics of the cone of rays generated from each
point.
.Ce
.H 3 "Burst Distance"
.Cs "/burst points/bursting parameters/burst-distance"
The \fIburst distance\fR parameter is modeled after the \fBBDIST\fR parameter
used by the Air Force's \*(G)\*F code.
.FS
The Air Force currently uses \*(G) to produce burst point libraries
for \*(P) from \*(F) descriptions.
.FE
The role of this parameter is overloaded,
however it was retained to aid \*(G) users in transitioning to the \*(b)
program.
If it is zero or negative,
then \fIinterior bursting\fR is enabled,
otherwise,
if it is greater than zero, \fIexterior bursting\fR will occur,
subject to certain conditions (see below).
The magnitude of this parameter is used to offset the burst point location
along the shotline relative to the geometry that triggered the burst.
.Ce
.H 4 "Interior Bursting"
.sp
\fIBurst armor\fR refers to a component whose ident code is found in
the list input by the
.Co burst-armor-file
command.
Similarly,
\fIburst air\fR refers to a component whose ident code is found in the
list input by the
.Co burst-air-file
directive.
If \fIinterior bursting\fR is enabled
and a \fIburst armor\fR component is encountered along a shotline
that is immediately followed by \fIburst air\fR,
then a burst point will be located the absolute value of \fIburst distance\fR
beyond the exit of the shotline from the component.
This means that if \fIburst distance\fR is zero,
the burst point will lie at the \fIburst armor/air\fR interface,
and if its -5.5,
the burst point will lie 5.5 units inside the air compartment from the
back surface of the armor.
.Ce
.H 4 "Exterior Bursting"
.sp
If \fIburst distance\fR is greater than zero,
the first component encountered along the shotline will trigger a
burst point,
regardless of its ident code,
that will be located \fIburst distance\fR
in front of the shotline \fIentry\fR point.
This technique simulates the behavior of a fragmenting munition with
a \fIstandoff\fR fuzing such that detonation is triggered before
the collision of the warhead with the target.
The \fIburst distance\fR is set to imitate the built in \fIstandoff\fR
of the warhead.
When employing \fIexterior bursting\fR methods,
\fIburst armor\fR and \fIburst air\fR are not used.
.sp
.H 3 "Sampling Cone Half Angle"
.Cs "/burst points/bursting parameters/cone-half-angle"
To limit the solid angle within which burst rays will be generated,
the user may specify a \fIcone half angle\fR.
This angle represents the degrees (in floating point) from the \fIaxis\fR of
the cone to its limiting surface.
The default value for the \fIcone half angle\fR is 45 degrees.
.Ce
.H 3 "Deflected Sampling Cone"
.Cs "/burst points/bursting parameters/deflect-spall-cone"
The spall cone axis is,
by default,
aligned with the shotline.
In reality,
the center of mass of the spall cloud would be between the shotline direction
and the exit normal of the shotline from the spalling component.
By aligning the spall cone axis with a vector halfway between the shotline
and the exit normal,
a narrower cone half angle can be used and still sample within the solid
angle of interest.
This technique can therefore be used to cut down on the number of rays
calculated without lowering the sampling density.
The
.Co deflect-spall-cone
command takes a \fIyes\fR or \fIno\fR argument about whether or not
to divert the cone axis.
.Ce
.H 3 "Number of Sampling Rays"
.Cs "/burst points/bursting parameters/max-spall-rays"
The sampling ray density within the spall cone is controlled by specifying
the maximum number of rays desired.
Due to the uniform distribution algorithm employed,
the number of rays calculated will be slightly less.
.Ce
.H 3 "Maximum Barriers"
.Cs "/burst points/bursting parameters/max-barriers"
For munitions known to have limited penetration capability,
the user may set a limit on the number of burst ray intersections reported.
The effect of setting this parameter is to reduce the size of the
burst point library (see \fBBurst Point Library\fR) by limiting the
number of components that will be reported per burst ray.
By default,
up to 100 components are reported,
as it is not expected that this number will be reached under normal
circumstances.
.Ce
.H 1 "Input File Options"
.H 2 "Target-Related Input Files"
.Cs "/target files"
This group of commands is for specification of target-specific input files.
.Ce
.H 3 "Target Data Base File"
.Cs "/target files/target-file"
The input of the target's \*(m) file is accomplished with this command.
Note that only one data base may be read in during a given \fIsession\fR.
If the user wishes to change the target once the
.Co execute
command has been given,
he must exit the \*(b) program and start a new \fIsession\fR.
.Ce
.Cs "/target files/target-objects"
After specifying the \*(m) file,
the user must list all of the objects in the \*(m) hierarchy that he
wishes to be included in his analysis.
The objects must be listed as arguments to one
.Co target-objects
command with spaces or tabs as separators.
Note that only one list of objects may be loaded per \fIsession\fR,
however,
they do not get loaded until the
.Co execute
command is given.
.Ce
.H 3 "Ident List Input Files"
.sp
\fIIdents\fR refer to the \fIregion ident\fR code from the \*(m) file.
Lists of \fIidents\fR may be specified singly or as ranges.
Individual \fIidents\fR must appear as one per line,
but ranges are specified by two numbers on a line that are
separated by one or more of the following characters: comma, hyphen, colon,
semicolon, space, or tab.
Figure 6 shows an example of such a file.
.DS
.sp
.so ids.mm
.FG "Ident List File Format"
.sp
.DE
.Ce
.H 4 "Burst-Armor and Burst-Air Ident Files"
.sp
.Cs "/target files/burst-armor-file"
.Cs "/target files/burst-air-file"
When interior burst points are to be generated along a shotline
(see \fBInterior Bursting\fR) a file of \fIburst armor\fR idents and
\fIburst air\fR idents must be input.
If a shotline intersects a component whose \fIident\fR has been input
as a \fIburst armor\fR and it is immediately followed by \fIburst air\fR
a burst point will be triggered.
.Ce
.H 4 "Critical Component Idents"
.Cs "/target files/critical-comp-file"
Whether interior or exterior bursting is being employed,
information about components hit by burst rays will only be output
for rays that hit \fIcritical components\fR.
The file name containing a list of \fIcritical component\fR idents must
therefore be input by this command if burst points are to be generated.
.Ce
.H 3 "Color Mapping Input File"
.Cs "/target files/color-file"
This command allows users to assign colors to component idents for graphics
options,
in particular,
the
.Co image-file
and
.Co plot-file
commands.
The format of this file is 5 numbers per line separated by blanks or tabs.
The first number is the low end of an ident range and the second number is
the high end of the range (both numbers are inclusive).
This range is mapped to the color specified by the last 3 numbers on the
line that are red, green, and blue components of the color (values for
these components must be between 0 and 255 inclusive).
Figure 7 shows an example of such a file.
.DS
.sp
.so cmap.mm
.FG "Color Mapping File Format"
.sp
.DE
.Ce
.H 2 "Project-Related Input Files"
.H 3 "Reading Session Files"
.Cs "/project files/read-input-file"
This command reads an \fIinput\fR file of commands.
These files can be generated manually by using a text editor or
saved from a \fIsession\fR file with the
.Co write-input-file
command.
See the section called \fBCommand Input\fR for the format of this
file.
.Ce
.H 3 "Shotline and Burst Point Input Files"
.sp
For an explanation of commands for reading in files of shotline or burst
point coordinates,
see the sections called \fBInput Discrete Shots\fR and
\fBInput Discrete Burst Points\fR.
.Ce
.H 1 "Output File Options"
.Cs "/project files"
The following commands will turn on optional output.
By default,
no output is produced except error logging (see \fBError Log\fR),
unless an output file is specified with the appropriate command.
Any combination of output options may be specified for a particular run.
Note that specifying an output file will cause an existing file with that
name to be truncated to zero length.
Therefore,
only one such command should be entered per \fIsession\fR for a particular file
name.
Multiple runs during a \fIsession\fR will append to the same files if
intervening commands to change the output file name are not given,
except for the graphics files as explained below.
Note that there is no way to append to a file created by a previous
\fIsession\fR of the \*(b) program,
but these files may be concatenated after the fact by using the
\*(U)
.Ud cat 1
utility.
.Ce
.H 2 "Burst Point Library File"
.Cs "/project files/burst-file"
This command will open the named file for creating a burst point library.
If the file exists,
it will be truncated by this command.
\fBAppendix A\fR describes the format for these files.
.Ce
.H 2 "Shotline File"
.Cs "/project files/shotline-file"
This command will open the named file for creating a shotline file.
If the file exists,
it will be truncated by this command.
\fBAppendix B\fR describes the format for these files.
.Ce
.H 2 "\*(U) Plot File"
.Cs "/project files/plot-file"
This command generates a \*(U) plot file,
using \*(B) extensions to the standard format.
This option is useful for examining the shotline and burst ray information
graphically as a three-dimensional vector plot.
Due to constraints inherent in the \*(U) plot format,
these plots must be displayed as a post-process step by using
a \*(B) plotting utility such as \fBpl-sgi\fR or \fBpl-fb\fR.
Because some of these display programs do not support multiple plots
per file,
the file name should be changed between runs.
Table 3 describes the color mapping associated with these plots.
.DF
.sp
.TB "Color Key for \s-1UNIX\s0 \*(Tm Plots"
.so plot.mm
.sp
.DE
.P
If the user has specified a color mapping file with the
.Co color-file
command,
then those colors will be used rather than the above colors for
all shotline/ray intersections.
.Ce
.H 2 "Frame Buffer Image"
.Cs "/project files/image-file"
This option will generate a color image that provides the user with
immediate feedback about a run.
The grid is displayed graphically and each cell location is dynamically
color coded to show its current status.
Table 4 describes the color mapping associated with the grid.
.DF
.sp
.TB "Color Key for Frame Buffer Image"
.so fb.mm
.sp
.DE
.P
In addition to the above cell colors, hits on critical components by burst
rays are depicted as a colored pixel projected into grid
space from the intersection point where the ray enters the component.
Colors for the components are mapped from ident numbers according to the
table specified by the user with the
.Co "/target files/color-file"
directive and shaded using a lighting model illuminated from the viewing
direction.
.H 2 "Grid File"
.Cs "/project files/grid-file"
This command will store each shotline coordinate generated during the run
as grid offsets.
These files can later be read in to replicate a previous run's grid or discrete
shots by using the
.Co read-2d-shot-file
command.
This capability is especially useful when \fIdithered\fR shotlines have been
used and it is desired that the same shotlines be used in another run.
Note that the shotline intersection information is not saved,
just the grid offsets for each shotline.
.Ce
.H 2 "Script File"
.Cs "/project files/write-input-file"
During a \fIsession\fR,
all commands are saved in a temporary file.
The
.Co write-input-file
command will create a \fIsnapshot\fR of this \fIsession\fR file,
that can later be used to recreate the current \fIsession\fR
up to the point when the file was written.
The \fIsession\fR or \fIinput\fR files
can later be used in one of two ways: either read in with the
.Co read-input-file
command,
or supplied on the standard input of the \*(b) program.
Note that the
.Co write-input-file
and
.Co read-input-file
commands will not be included in the \fIinput\fR files,
but the commands read in by the latter \fIwill\fR.
.Ce
.H 2 "Error Log"
.Cs "/project files/error-file"
This command is useful to save errors in a log file and prevent copious
ray tracer diagnostics from scrolling by on the screen.
This option is especially useful if using the batch mode of execution
so that the terminal is not tied up by program output. 
If no error log is specified,
diagnostic messages will appear in the scrolling window or,
if in batch mode,
on the \*(b) program's standard error output.
.Ce
.H 2 "Histogram File"
.Cs "/project files/histogram-file"
This command generates a frequency histogram to the named file.
The file format is simply one number per line; each number is a count
of critical components hit by an individual burst ray.
This file can easily be post-processed to display a histogram,
for instance,
how many rays hit zero, one, two, three, etc. components.
.Ce
.H 1 Conclusion
.P
There is a large, and rapidly growing, collection of highly detailed \*(m)
models due to the widespread use of the highly portable \*(B) package.
The \*(b) program gives the \*(F) community the option of analyzing \*(m)
models.
This, along with the \*(F) to \*(m) translator will permit an easy transition
of the Air Force to the \*(B) system.
Then,
productivity in the target description arena,
that has long been a bottleneck to the vulnerability and lethality analyst,
will increase dramatically.
Another benefit will be increased accuracy of results due to the superior
detail and accuracy afforded by a true combinatorial solid geometry modeler.
.H 1 Acknowledgements
.P
The author would like to thank Leonard E. Bruenning, Jr. of Sverdrup
Technology, Inc. for his thorough, concise and well-illustrated
documentation\*F of the requirements to be met by the \*(b) program
and for his courteous and professional manner in fielding related questions.
.FS
Leonard E. Bruenning,
"Interface Control Document for BURST Ray Generator Computer Program",
\fBTechnical Report\fR,
prepared for: Department of the Air Force,
Armament Division,
Eglin AFB,
FL 32542-5000,
14 December 1988.
.FE
.H 1 Appendices
.P
Both burst point library files and shotline files are composed
of single-line records.
Each line begins with a digit that identifies the type of record it represents.
Both file formats are composed of one or more \fIruns\fR that begin with a
\fIrun header\fR.
Each shotline that intersects the target will result in a
\fIshotline header\fR that is followed by one or
more \fIshotline intersection\fR records.
These record types exist for both file formats,
but they do not contain the same information.
Burst point library files have two additional record types,
\fIburst ray headers\fR and \fIburst ray intersections\fR.
If a burst point occurs along a shotline,
the \fIburst ray header\fR will immediately follow that shotline's last
intersection record.
Each \fIburst ray header\fR records will be immediately followed by
\fIburst ray intersection\fR records,
one for each ray that strikes a \fIcritical component\fR.
.bp
.H 2 "Appendix A:  Burst Point Library File Format"
.P
Below is a description of the format of the burst point library files that
are generated if the
.Co burst-file
command is used.
.DS
.so bpl.mm
.DE
.bp
.H 2 "Appendix B:  Shotline File Format"
.P
Below is a description of the format of the shotline files that are
generated if the
.Co shotline-file
command is used.
.DS
.so shotlines.mm
.DE
.de TX
.ce
TABLE OF CONTENTS       \" define header for TOC page
..
.de TY          \" suppress default TOC header
..
.TC
.\" .CS
