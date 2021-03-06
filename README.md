# CDimg|tools

**NOTE:**
This code was copied and uploaded from the [now dead repo over at gna.org](https://web.archive.org/web/20170203202531/http://gna.org/projects/cdimgtools/).

All credits go to the original author and only a few changes were made to fix compilation issues.

---

CDimg|tools is a set of command line tools to inspect and manipulate CD/DVD
optical disc images of formats uncommon on free UNIX-like systems (like
GNU/Linux or BSD).

One sort of tool is concerned with a certain special image file format,
possibly a non open one.  It can print information about an image file in this
format, or extract parts of its data (e.g. audio tracks or disc sessions) to
files in ``raw'' format, so that they can be further processed by more widely
available tools (e.g. for the purpose of playing audio, reading files, burning
tracks or sessions to optical media).

- NRG is the only supported format for the present.

Another sort of tool is concerned with the processing of files in a ``raw''
format.  It can convert between different variations of formats of the same
type, or convert from a raw format to an equivalent well-known self-describing
format, or modify the raw data without changing its format.  For the present it
is possible:

- to demultiplex RAW+96 image files containing both stream data and sub-channel
  data,

- to decrypt CSS-scrambled VOB files,

- and to decrypt CSS-scrambled DVD Video image files.

CDimg|tools was mainly written under a GNU/Linux OS but should work under other
UNIX flavours, and perhaps even on other platforms, albeit maybe only partially
(see the file link:INSTALL.html['INSTALL'] for a list of dependencies).


## Commands in the package

linkpage:raw96cdconv[1]::
	This command is a Perl script that demultiplexes/multiplexes stream data
	(audio, raw data or .iso format data) and sub-channel data from a RAW+96
	image file.  It handles optical disc images containing stream data and
	sub-channel data of a given sector at contiguous positions; this kind of
	image file may be either directly created by a ripping program like
	readcd(1) or cdrdao(1), or extracted from another image file by nrgtool(1).

linkpage:nrgtool[1]::
	This command is a Perl script that reads .nrg images (created by Nero),
	prints the metadata about the image, and extracts, for each track, the raw
	data to a separate file; disc types such as CDDA, CD-extra and
	(multi-session) CD-ROM/DVD-ROM are supported.

linkpage:cssdec[1]::
	This command decrypts CSS-scrambled VOB files or streams found on DVD Video
	discs; the title key is obtained transparently by libdvdcss.

linkpage:dvdimgdecss[1]::
	This command decrypts CSS-scrambled DVD Video discs or image files; it
	writes a similar image file containing the same data at the same location;
	the UDF filesystem of the disc is left intact.


## Resources

- The official homepage is at link:https://gna.org/projects/cdimgtools[Gna!].
  There you might find up-to-date information and releases.

- UNIX manual pages that contain usage examples are available for each command.

- Known bugs and limitations are mentioned in the section about <<bugs, issues>>.

- Release notes listing important user-visible changes are available in a
  separate file link:NEWS.html['NEWS'].


[[dependencies]]

## Installation instructions

Download and installation instructions and a list of dependencies can be found
in the file link:INSTALL.html['INSTALL'].


// Information about bugs and limitations can be found in the file 'BUGS'.
include::BUGS[]


## Credits & License

CDimg|tools was written by G??raud Meyer (g_raud chez gna.org).

CDimg|tools is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License version 2 as published by the Free
Software Foundation.

This package is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

The full text of the license can be found in the root directory of the project
sources, in the file 'COPYING'.  Otherwise see <http://www.gnu.org/licenses/>.
