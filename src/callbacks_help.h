/*
 *  Copyright (C) 2003 Jan C. Depner
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  $Id: callbacks_help.h,v 1.46 2007/06/29 17:17:58 jdepner Exp $
 */

#include "support.h"

char general_help[] = {N_(
"    JAMin is the JACK Audio Mastering interface.\n\n\
    Web site: <http://jamin.sourceforge.net>\n\n\
    JAMin is designed to perform professional audio mastering of any number \
of input streams.  It consists of a number of tools to do this: a 1024 band \
hand drawn EQ with modifiable parametric controls, a 31 band graphic EQ, \
3 band compressor, 3 band stereo width control, lookahead limiter, boost, and \
a number of other features.\n\n\
    Steve Harris is the JAMin principle author and team leader.\n\n\
    SourceForge CVS developers, in alphabetical order:\n\n\
    Jan Depner\n\
    Steve Harris\n\
    Jack O'Quin\n\
    Ron Parker\n\
    Patrick Shirkey\n\n\
    JAMin is released under the GNU General Public License and is copyright \
(c) 2003 J. Depner, S. Harris, J. O'Quin, R. Parker, and P. Shirkey. \n\
    This program is free software; you can redistribute it and/or modify it \
under the terms of the GNU General Public License as published by the Free \
Software Foundation; either version 2 of the License, or (at your option) any \
later version.\n\
    This program is distributed in the hope that it will be useful, but \
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or \
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for \
more details.\n\
    You should have received a copy of the GNU General Public License along \
with this program; if not, write to the Free Software Foundation, Inc., 675 \
Mass Ave, Cambridge, MA 02139, USA.\n")
};


char prerequisites_help[] = {N_(
"    JAMin would not be functional without many other open source packages \
and we thought that it would only be right to list them here:\n\n\
    JACK - JACK Audio Connection Kit\n\
    http://jackit.sourceforge.net\n\n\
    SWH plugins - Steve Harris' LADSPA plugins\n\
    http://plugin.org.uk\n\n\
    LADSPA - Linux Audio Developer's Simple Plugin API\n\
    http://www.ladspa.org\n\n\
    FFTW - Fastest Fourier Transform in the West\n\
    http://www.fftw.org\n\n\
    libsndfile - Erik de Castro Lopo's sound file I/O\n\
    http://www.zip.com.au/~erikd/libsndfile\n\n\
    GTK - The GIMP Toolkit\n\
    http://www.gtk.org\n\n\
    ALSA - Advanced Linux Sound Architecture\n\
    http://www.alsa-project.org\n\n\
Many thanks to all of the authors of these packages!\n")
};


char help_help[] = {N_(
"    The File menu has the standard options available.  Save and Save As \
allow you to save the scenes and settings for a session to a .jam file.  \
By default these are saved in the $HOME/.jamin directory but may be saved \
anywhere.\n\
    The Edit menu has undo and redo options for changes to the settings.  \
It also has the Preferences entry to bring up the Preferences dialog.  \
The Preferences dialog allows you to change the default values for many \
of JAMin's settings.  More detailed help is available in the Preferences \
dialog.\n\
    The Ports menu allows you to set the input and output ports for JAMin.\n\n\
    For help on the rest of the GUI, context specific help can be obtained \
by moving the mouse pointer into one of the tool areas (compressor, EQ, \
limiter, input, etc) and pressing <Shift><F1>.\n")
};



char preferences_help[] = {N_(
"    This dialog allows you to change many of JAMin's default settings.\n\n\
    Graphic EQ : Set the minimum and maximum levels for the hand-drawn EQ \
(HDEQ) and the graphic EQ (GEQ).\n\n\
    Crossfade : Set the length of the crossfade time when changing between \
scenes or using the undo/redo functions.\n\n\
    Warning Level : Set the input and output meter warning level.\n\n\
    Spectrum : Set the source for the spectrum displays in the HDEQ and the \
Spectrum tab.  Also, set the spectrum update frequency for both of these \
tabs.\n\n\
    Crossover Type : Choose the type of crossover.  Fast Fourier Transform (FFT) \
or Infinite Impulse Response (IIR).  IIR should only be used for testing as \
your results will be substantially inferior to the FFT crossover.\n\n\
    RMS Sample Size : Set the time slice size to be used for the RMS meter \
displays.  The number of samples will change as you modify the time.\n\n\
    Band Delays : Set delays for the low and mid bands.  The maximum delay \
for the low band is 2 milliseconds and the maximum delay for the mid band is \
0.5 milliseconds.  These delays are used for compensating for for the different \
low and mid frequency drivers in a speaker system.  This is sometimes nice for \
mastering.  The delays will not bgo into effect unless the Delay buttons under \
the low and mid band compressor control areas are selected.\n\n\
    Colors : Set the colors for most of JAMin's displays.  These colors \
are saved in the file $HOME/.jamin/jamin-defaults.  This is done whenever you \
exit from JAMin.  You can actually edit this file and change the colors by \
hand.  They are just RGB values in the 0-65535 range but it's much easier to \
use the GUI ;-).\n\n\
    Limiter : Select either Steve Harris' fast-lookahead-limiter plugin or \
Sampo Savolainen's foo-limiter.  Note that there may be artifacts if you push \
the foo-limiter too hard.\n\n\
    Output Meter Numeric Display : Set whether the output meter numeric displays \
for output and RMS display peak values or the current level values.\n")
};


char hdeq_help[] = {N_(
"    The hand drawn EQ (HDEQ) allows the user to draw the EQ curve using the \
mouse.  The curve is then splined to fill 1024 EQ bands.  There are a number \
of other options available in the HDEQ.  There are user defined notches that \
act as parametric EQ controls.  There are also crossover controls that will allow \
the user to change the compressor crossover points.  The following is a quick \
guide to using the HDEQ:\n\n\
    In the background window - left click and release to begin drawing the \
curve.  Left click again to end the curve.  You can define any portion of the \
curve, you don 't have to define the entire curve.  Drawing can be done from \
left to right or right to left.  If you try to reverse direction while \
drawing, the data in the reverse direction will be ignored.  You can discard \
the curve that you are drawing by clicking the middle button or by clicking the \
right button and selecting Cancel from the popup menu that appears.  \
Clicking the right mouse button in the HDEQ will pop up a menu with four \
entries:\n\n\
    Reset HDEQ curve\n\
    Release parametric EQ controls\n\
    Help\n\
    Cancel\n\n\
The first entry will reset the entire EQ curve to its original \
(flat) setting.  The second entry will leave the EQ curve as is but will release \
the three normal (non-shelving) parametric EQ controls to be used again (this \
effectively gives you an unlimited number of parametric EQ controls).  The third \
entry (Help) will display this help information.  The fourth entry (Cancel) will \
discard any hand drawn EQ line that you are in the process of drawing.  If you \
are not drawing a line it does nothing.\n\n\
    Over the crossover bar handles - left click and hold to drag the \
crossover bars.\n\n\
    Over the notch handles - left click and hold to drag the notch center or \
cuttoff frequency and gain.  <Ctrl>-left click will reset the notch to 0.  If \
you hold the shift key while adjusting the notch handle it will only move in \
the Y (gain) direction.\n\n\
    Over the notch width handle - left click and hold to widen or narrow the \
notches, except the high and low cutoff notches which have no width handles.\n")
};


char crossover_help[] = {N_(
"    The adjustable crossover is used to split the entire audible range into \
three sections.  It applies to the compressors and stereo widths. If the \
crossovers are set to 500Hz and 5KHz then the first stereo width control and \
compressor works for 25-500Hz, the second for 500Hz-5KHz, and the last for \
5KHz-20KHz.  The crossover has no effect on the HDEQ unless one or more bands \
are soloed.  The crossover bars that are visible in the HDEQ serve as a visual \
reference.\n")
};


char geq_help[] = {N_(
"    The graphic EQ (GEQ) can be used to set gain for specific bands of the \
audio spectrum.  The center of the band is annotated at the bottom of each \
fader.  Setting a fader in the GEQ will override the HDEQ in the immediate \
vicinity of the changed fader and cause that curve to be redrawn.  It will \
not override the parametric notch settings in the HDEQ.  Clicking the right \
mouse button on a GEQ control will reset the value to 0.0.\n")
};


char input_help[] = {N_(
"    The input section allows you to set the input gain to the JAMin system.  \
You can also pan the input left or right.  Clicking the right mouse button on \
the gain/balance control will reset the value to 0.0/centre.  Clicking the \
right mouse button on the meters will reset the peak indicator to the current \
level.\n")
};


char spectrum_help[] = {N_(
"    The spectrum display shows the power spectrum of the signal in bands \
that correspond to the frequency bands in the 30 band EQ (GEQ). In addition \
it displays the maximum value reached for each band as a (by default) blue \
line.\n")
};


char comp_curve_help[] = {N_(
"    The compressor curves show a graphical representation of the compression \
for each compressor band.  The bands are defined by the crossover that can \
be set using the crossover faders or the crossover bars in the HDEQ.  The \
X-axis shows the input in db while the Y-axis shows the output in db.  The \
scale is from -60 to 0 in X and -60 to +30 in Y.\n")
};


char comp_help[] = {N_(
"    The compressors allow you to set compression parameters for each \
compressor band.  The bands are defined by the crossover that can be set \
using the crossover faders or the crossover bars in the HDEQ.  The parameters \
are, from left to right:\n\n\
    A - attack in milliseconds\n\
    R - release in milliseconds\n\
    T - threshold in db\n\
    r - compression ratio (N:1)\n\
    K - knee (0.0 [hard] to 1.0 [soft])\n\
    M - makeup gain in db\n\
    Note that the value label for makeup gain is also the automatic makeup \
gain button.  Pressing this will cause JAMin to try to approximate the \
optimum makeup gain for the other settings.  When pressed you cannot \
manually adjust the makeup gain however, adjustments to the threshold or \
ratio will cause the makeup gain to change.\n\
    A full explanation of the use of these parameters is covered in the user \
manual.\n\
    Compressor controls can be \"ganged\" by clicking on the control label in \
the desired compressor band windows.  When they are ganged moving one control \
will move all other ganged controls by the same amount.  To ungang just click \
on the label again.  The labels change to the band color to indicate that \
they are ganged.\n")
};


char stereo_help[] = {N_(
"    The stereo width controls define the apparent 'wideness' of the stereo \
signal for each of the three bands.  The bands are defined by the crossovers \
that can be set using the crossover faders or the crossover bars in the \
HDEQ.  More negative values decrease the 'width' while positive values \
increase the 'width'.  Clicking the right mouse button on the stereo width \
control will reset the value to 0.0.\n")
};


char limiter_help[] = {N_(
"    The lookahead limiter is a brickwall limiter that will not allow the \
output to exceed the set level.  It \'looks ahead\' by the specified amount \
in order to make a smooth transition as it nears the limit level.  Clicking \
the right mouse button on the input or limit control will reset the value to \
0.0.  Clicking the right mouse button on the input or limit meters will \
reset the peak indicator to the current level.\n")
};


char boost_help[] = {N_(
"    The boost control allows the user to add 'tube like' gain to the output \
signal.  Use to taste (New England mild to Cajun spicy).  Clicking the right \
mouse button on the boost control will reset the value to 0.0.\n")
};


char output_help[] = {N_(
"    The output control allows you to decrease the output level.  The upper \
level is 0dB.  Clicking the right mouse button on the output control will \
reset the value to 0.0.  Clicking the right mouse button anywhere on the meters \
will reset the peak indicators for both the normal and RMS meters to their \
current level.  The level indicator boxes above the meters display either \
peak value or the current value for the main output meters and the RMS \
level meters.  Which value is displayed can be selected in the Preferences \
dialog that is accessible from the Edit->Preferences menu entry.  You can reset \
the meter peak inicators (and values if that is what is displayed) by clicking \
in the individual level indicator boxes.\n")
};


char eq_options_help[] = {N_(
"    The EQ Options dialog allows you to set the minimum and maximum gain \
level in dB for both the HDEQ and GEQ.  You can also set the source of the \
spectrum for both the HDEQ and spectrum windows.  Spectrum update frequency \
can be set here as well.  Up to 10 per second (default) and down to 0 \
(disabled).\n")
};


char spectrum_options_help[] = {N_(
"    This allows you to set the input to the spectrum computation for both \
the Spectrum window and the HDEQ.  The default is Post EQ.  The other options \
are Pre EQ, Post compressor, and Output.\n")
};


char transport_controls_help[] = {N_(
"    The transport controls give you the usual tape transport type controls.  \
These are useful if you are using other JACK enabled applications that honor \
the JACK transport control functions (ecamegapedal, Ardour). Note that there \
is no stop button (use pause). Some JACK and system status information is \
available to the right of the transport buttons.  The fast forward and \
reverse buttonsa will move the transport forward or back by five seconds.\n")
};


char bypass_help[] = {N_(
"    This check box allows you to bypass all of JAMin's processing so that \
you can quickly listen to the difference between the processed and unprocessed \
sound.  The keyboard accelerator for this button is the 'b' key.\n")
};


char scenes_help[] = {N_(
"    Scenes are used to save and recall an entire group of JAMin parameter \
settings during a session.  After setting all of the parameters for a \
specific section of music (verse, chorus, bridge) or a complete song you can \
right click on a scene button, use the menu to 'Set' that scene, then use the \
'Name' entry of the menu to enter a name for that scene (Back Alley Fugue - \
verse).  You can recall these settings by left clicking on the scene button.  \
A bright green button means that the scene associated with this button is \
currently the active scene.  A dull green button signifies that the scene is \
loaded but not active.  A red button means that the scene has no settings \
loaded.  A bright yellow button means that this scene is active but the \
settings have been changed.  If you want to save the settings you must use \
the 'Set' entry in the button's right click menu.  You can clear settings \
from a button using the right click menu and the 'Clear' entry.\n\n\
    The keyboard accelerators for the scene buttons are the number keys, \
1 through 0 for scenes 1 through 10.  <Shift>-1 through <Shift>-0 will access \
scenes 11 through 20.  For example, pressing the 1 key will cause scene 1 to \
become active.  Pressing <Shift>-5 will cause scene 15 to become active.  The \
ALT modifier can be used to assign settings to a scene button (instead of \
using the scene button menus).  Pressing <Alt><Shift>-5 will assign the \
current settings to scene button 15 (you may still want to change the name).  \
The CTRL modifier can be used to clear a scene button.  Pressing <Ctrl>-4 \
will clear scene button 4.  \n\n\
    Note that the keypad keys may be used but only for scenes 1 through 10 \
with Num Lock on.  Using the keypad with Num Lock off will access the cursor \
keys (which can be used to control some aspects of the GUI).  Using <Shift> \
with the keypad with Num Lock on will also access the cursor keys.\n")
};


char eq_bypass_help[] = {N_(
"    This button allows you to bypass all EQ processing.  It will have no \
effect on any of the other controls.\n")
};


char band_button_help[] = {N_(
"    The solo buttons allow you to listen to selected bands while muting the \
other bands.  Selecting two of the solo buttons effectively mutes the \
remaining band.  The per band bypass buttons allow you to bypass compression \
on the selected bands.\n")
};


char limiter_bypass_help[] = {N_(
"    This button allows you to bypass limiting.  It has no effect on any of \
the other controls.\n")
};


char limiter_plugin_warning[] = {N_(
"    Changes to the limiter plugin will not go into effect until JAMin is \
restarted.\n")
};


char keys_help[] = {N_(
"    Keyboard accelerators are available for many of the functions in \
JAMin:\n\n\
\tb\t\t\t-\tGlobal bypass\n\
\tSpace\t\t-\tToggle play and pause\n\
\tHome\t\t-\tPosition transport to beginning\n\
\t<\t\t\t-\tMove transport backwards 5 sec.\n\
\t>\t\t\t-\tMove transport forwards 5 sec.\n\
\t1-0\t\t\t-\tSelect scene 1-10\n\
\t<Shift>-1-0\t-\tSelect scene 11-20\n\
\t\t\t\t\t<Alt> will set scene, <Ctrl> will clear\n\
\t<Ctrl>-o\t\t-\tOpen session file\n\
\t<Ctrl>-s\t\t-\tSave to current session file\n\
\t<Ctrl>-a\t\t-\tSave session file as new session file\n\
\t<Ctrl>-q\t\t-\tQuit\n\
\t<Ctrl>-u\t\t-\tUndo\n\
\t<Ctrl>-r\t\t-\tRedo\n\
\t<Ctrl>-h\t\t-\tGeneral help\n\
\tF1\t\t\t-\tSet the notebook tab to HDEQ\n\
\tF2\t\t\t-\tSet the notebook tab to 30 band EQ\n\
\tF3\t\t\t-\tSet the notebook tab to Spectrum\n\
\tF4\t\t\t-\tSet the notebook tab to Compressor \n \t\t\t\t\tcurves\n\
\tF5\t\t\t-\tSet the notebook tab to the EQ \n \t\t\t\t\tOptions\n\
\t<Ctrl>-k\t\t-\tKeyboard accelerator help (this \n \t\t\t\t\tscreen)\n\
\t<Ctrl>-j\t\t-\tAbout JAMin\n")
};
