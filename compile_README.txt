1. Capture Client Compilation Instructions
------------------------------------------
For convenient compilation, we have provided a makefile to compile the Capture Client.

2. Prerequisites
----------------
One needs the following software installed:
	- the Windows 2003 WDK  (available at http://www.microsoft.com/whdc/devtools/WDK/default.mspx)
	- the Windows Server 2003 Platform SDK (available at http://www.microsoft.com/downloads/details.aspx?FamilyId=A55B6B43-E24F-4EA3-A93E-40C0EC4F68E5&displaylang=en)
	- the Boost C++ libraries 1.34.0 (available at http://www.boost.org/)
	- WinPcap 4.0.1 Developer's Pack 
	- Visual C++ 2005 Express Edition (available at http://msdn.microsoft.com/vstudio/express/visualc/default.aspx)
	- expatpp XML library v1.95.6 (available at http://www.oofile.com.au/downloads.html#DownloadXML)
	- DDK Build v7.0 (available at http://www.osronline.com/article.cfm?article=43)
	- NSIS 2.29 (available from http://nsis.sourceforge.net/)

Ensure that the following environment variables are set:
	INCLUDE  - should be set to point to relevant include directories of Boost C++ libraries, Windows Server 2003 Platform SDK, Windows 2003 WDK, expatpp XML library and the WinPcap Developer's Pack. For example, INCLUDE could be set to: C:\Boost\include\boost-1_34;C:\Program Files\Microsoft Platform SDK\Include;C:\WinDDK\6000\inc\api;c:\expatpp\src_pp;C:\WpdPack\Include;C:\expatpp\expat\lib
	LIB 	- should be set to point to relevant lib directories of Boost C++ libraries, Windows Server 2003 Platform SDK, Windows 2003 WDK, expatpp XML library. For example, LIB could be set to: C:\Boost\lib;C:\Program Files\Microsoft Platform SDK\Lib;C:\WinDDK\6000\lib\wxp\i386;C:\expatpp\expat\lib;C:\expatpp\vc_pp\expatpp\ReleaseMT
	WNETBASE - should be set to the Windows 2003 WDK install directory, for example: c:\WinDDK\6000
	
Ensure that the following directories are in the PATH:
	VCINSTALLDIR\bin
	WNETBASE\bin
	path to DDKBUILD.cmd (of DDK Build v7.0)
	path to makensis.exe (of NSIS 2.29)
	
3. Compilation
--------------
1. Open a Visual Studio 2005 command prompt
2. cd into the captureclient directory and execute 'nmake release-hpc' or 'nmake release-bat'. 
3. After successful compilation, the built Capture Client setup file is located in the main directory. Follow the instructions for installing the client described in README.txt.	
