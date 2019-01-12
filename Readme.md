# GbxDump #

## GBX file header viewer application ##

GbxDump is a little Windows application to indicate the contents of the file header of mainly Maps, Challenges, Replays, Packs, Blocks, Objects and Items used by the [Nadeo](https://nadeo.com/) game engine GameBox (\*.Gbx files). The tool is also able to display the file header of DirectDraw Surface files (\*.dds).

**Main features:**
- Displays general track properties
- Displays external dependencies
- Displays the author comment
- Shows and exports the thumbnail
- Online interface to the TMX and MX trackbase
- Online interface to the Dedimania records database

![Screenshot of GbxDump](http://www.wolfgang-rolke.de/gbxdump/gbxdump.jpg)

After installation the context menu of GameBox (Gbx) files has an additional entry "GbxDump". Using this command you can view into this Gbx file. You can also run the tool from the start menu and open any Gbx file within the program (Start » Programs » Gbx File Dumper). Nevertheless, the easiest way is to use drag-and-drop from Microsoft Windows Explorer. Then several files can immediately be analyzed at once.

GbxDump shows lot of technical data (like block identifiers and sizes) to support all types of Gbx files and to detect future changes of the file format. But the provided information can still be very useful for map builders, title producers, server admins and normal players.

**For example:**
- All map properties are indicated together at one place using only two mouse clicks.
- Links to external dependencies could be easily tested by copy'n'paste them into a browser.
- The dependence between maps to titles can be checked (via title ID).
- The Lightmap Cache hash is useful to discover the lightmap file in the cache folder.
- The T/MX map ID, the numbers of awards and records could be retrieved from T/MX or Dedimania.
- The thumbnail of (Macro-) Blocks, Objects and Items supplies a good indication of the file content.
- The used texture compression of a DDS file could discover an incompatibility with DirectX9.

The tool was originally used to examine the [\*.Gbx](https://wiki.xaseco.org/wiki/GBX) and [\*.pak](https://wiki.xaseco.org/wiki/PAK) file formats. Both formats have been well documented by several members of the Trackmania community in the [Mania Tech Wiki](https://wiki.xaseco.org/). The file parsers of GbxDump are almost completely based on the information of this wiki.

## Status of the project ##

The development of the application is basically finished. It is not intended to add many new functionalities or to change the structure of the source code significantly. However, the project will continue to be maintained. For example, there will be updates if changes to the header of Gbx files are known in the future.

For a long time, the to-do list contained the further structuring of the Gbx reader using C++ classes. Due to a lack of OOP practice, however, this has not yet been achieved. For this reason, only a size limited static table is used to store the names for identifiers. But this limitation is not a problem for processing the data in the file header.

The application displays [all known information](https://wiki.xaseco.org/wiki/GBX) from the file header and the contents of the reference table. It was never planned to display content from the body of Gbx files.  
The main reason for this was the formerly proprietary license of GbxDump. It was not compatible with the LZO library, which is necessary to decompress the file body. In addition, complete information about the structure of the complex MediaTracker block, which is required for proper data parsing, is still missing. Furthermore, the file body may contain information that is not intended for the public.

## Building the project ##

This is a generic C/C++ Win32 desktop project created with Visual Studio 2005. The workspace consists of the main project GbxDump, the subproject libjpeg with the [Independent JPEG Group's JPEG software](http://www.ijg.org/) and two Setup projects for 32 and 64 bit. The [Microsoft Windows SDK 7.1](https://www.microsoft.com/en-us/download/details.aspx?id=8279) for Windows 7 and .NET Framework 4 is used as the platform toolset. There are no other dependencies or prerequisites.

To support all target platforms down to Windows NT 4.0, the project has not yet been migrated to newer versions of Visual Studio. In addition, there are problems with the Setup projects starting with Visual Studio 2012 (see below).

However, to create the project with newer versions of Visual Studio, please note the following points:
- For all configurations, the Windows XP platform toolset "Visual Studio 20xx - Windows XP (v1xx_xp)" should be selected.
- All manifest files that Visual Studio manages on its own must be removed from the project configurations. For example, starting with Visual Studio 2008, the reference to the file "TrustInfo.manifest" has to be removed and starting with Visual Studio 2010, the reference to the file "DeclareDPIAware.manifest" has to be removed, etc. The corresponding metadata must then be set in the project settings.
- Starting with Visual Studio 2012, Setup projects are no longer provided out-of-the-box. For each version, however, a corresponding extension is available for download in the [Visual Studio Marketplace](https://marketplace.visualstudio.com/vs).

Note: The Setup projects from the Marketplace are unfortunately not completely bug free. For example, the installation of the created MSI files fails under Windows XP and older. And after restarting the IDE, the Setup project no longer recognizes the active configuration correctly. A workaround is to create the Setup for each configuration one after the other. To do this, first change the configuration e.g. from "Debug (Win32)" to "Release (Win32)" and then restart the IDE. Now you can create the Setup for "Release (Win32)" (but not for other configurations). Then change the configuration to e.g. "Release (x64)" and restart the IDE. Now you can create the Setup for "Release (x64)", etc. In the Project Explorer, the icon of the Setup project indicates whether the configuration has been loaded correctly.

## Contributing ##

The project was mainly published for reference purposes. However, contributions leading to better code are very welcome.
New functionalities are only included if the ratio of costs (source code, external libraries, resources) to benefits is appropriate. More extensive changes require a fork of the project.
See also the section [Status of the project](#status-of-the-project) above.

## License ##

Copyright (c) 2010 - 2019 by Electron

Licensed under the [EUPL](https://joinup.ec.europa.eu/software/page/eupl), Version 1.2 - see the [Licence.txt](Licence.txt) file for details.

This software is based in part on the work of the Independent JPEG Group.

## Links ##
- [Gbx File Dumper](http://www.wolfgang-rolke.de/gbxdump/)
- [Gbx file format](https://wiki.xaseco.org/wiki/GBX)
- [Microsoft Windows SDK for Windows 7 and .NET Framework 4 (VS2005/VS2008/VS2010)](https://www.microsoft.com/en-us/download/details.aspx?id=8279)
- [Microsoft Visual Studio 2013 Installer Projects](https://marketplace.visualstudio.com/items?itemName=UnniRavindranathan-MSFT.MicrosoftVisualStudio2013InstallerProjects)
- [Microsoft Visual Studio 2015 Installer Projects](https://marketplace.visualstudio.com/items?itemName=VisualStudioProductTeam.MicrosoftVisualStudio2015InstallerProjects)
- [Microsoft Visual Studio 2017/2019 Installer Projects](https://marketplace.visualstudio.com/items?itemName=VisualStudioProductTeam.MicrosoftVisualStudio2017InstallerProjects)
- [Independent JPEG Group's JPEG software](http://www.ijg.org/)
- [European Union Public Licence 1.1 & 1.2](https://joinup.ec.europa.eu/software/page/eupl)
- [EUPL Guidelines for users and developers](https://joinup.ec.europa.eu/collection/eupl/guidelines-users-and-developers)
- [Managing copyright information within a free software project](https://softwarefreedom.org/resources/2012/ManagingCopyrightInformation.html)
