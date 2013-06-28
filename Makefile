##############################################################################
##
##  Capture-HPC
##


INSTALL_DIR = $(MAKEDIR)\install
!CMDSWITCHES +s
!CMDSWITCHES +i

all: 
	echo "CaptureClient make script."
	echo "There are several targets defined in this make script:"
	echo "   - clean       - will clean out all files not belonging to the source. This is a"
	echo "                   good target to run if one would like to build the solution from"
	echo "                   scratch."
	echo "   - release-hpc - will build the capture-hpc solution from scratch. The resulting"
	echo "                   files will be located in the install directory."
	echo "   - release-bat - will build the capture-bat solution from scratch. The resulting"
	echo "                   files will be located in the install directory."
	
release-hpc: clean \
			 prepare \
			 build \
			 copy-hpc \

release-bat: clean \
			 prepare \
			 build \
			 copy-bat \

prepare :
	if not exist $(INSTALL_DIR) mkdir $(INSTALL_DIR)
	if not exist $(INSTALL_DIR)\plugins mkdir $(INSTALL_DIR)\plugins

build : 
	@vcbuild CaptureClient.sln /u

copy-bat :
	rmdir $(MAKEDIR)\install\plugins
	copy $(MAKEDIR)\Readme-BAT.txt $(INSTALL_DIR)\Readme.txt
	copy $(MAKEDIR)\COPYING $(INSTALL_DIR)
	copy $(MAKEDIR)\7za.exe $(INSTALL_DIR)
	copy $(MAKEDIR)\ExclusionLists\*exl $(INSTALL_DIR)
	copy $(MAKEDIR)\Release\CaptureClient.exe $(INSTALL_DIR)\CaptureBAT.exe
	copy $(MAKEDIR)\FileMonitorInstallation.inf $(INSTALL_DIR)
	copy $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\ProcessMonitor\i386\*.sys $(INSTALL_DIR)
	copy $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\RegistryMonitor\i386\*.sys $(INSTALL_DIR)
	copy $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\FileMonitor\i386\*.sys $(INSTALL_DIR)
	copy $(MAKEDIR)\SetupScript\CaptureBAT-Setup.nsi $(INSTALL_DIR)
	cd $(INSTALL_DIR)
	makensis.exe CaptureBAT-Setup.nsi
	cd ..
	move $(INSTALL_DIR)\CaptureBAT-Setup.exe $(MAKEDIR)
		
copy-hpc :
	copy $(MAKEDIR)\Readme-HPC.txt $(INSTALL_DIR)\Readme.txt
	copy $(MAKEDIR)\COPYING $(INSTALL_DIR)
	copy $(MAKEDIR)\7za.exe $(INSTALL_DIR)
	copy $(MAKEDIR)\ApplicationConfig\Applications.conf $(INSTALL_DIR)
	copy $(MAKEDIR)\ExclusionLists\*exl $(INSTALL_DIR)
	copy $(MAKEDIR)\Release\CaptureClient.exe $(INSTALL_DIR)
	copy $(MAKEDIR)\Release\plugins\Application_*.dll $(INSTALL_DIR)\plugins
	copy $(MAKEDIR)\FileMonitorInstallation.inf $(INSTALL_DIR)
	copy $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\ProcessMonitor\i386\*.sys $(INSTALL_DIR)
	copy $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\RegistryMonitor\i386\*.sys $(INSTALL_DIR)
	copy $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\FileMonitor\i386\*.sys $(INSTALL_DIR)
	copy $(MAKEDIR)\SetupScript\CaptureClient-Setup.nsi $(INSTALL_DIR)
	cd $(INSTALL_DIR)
	makensis.exe CaptureClient-Setup.nsi
	cd ..
	move $(INSTALL_DIR)\CaptureClient-Setup.exe $(MAKEDIR)
	

clean:
	del $(MAKEDIR)\CaptureClient-Setup.exe
	del $(MAKEDIR)\ApplicationPlugins\InternetExplorer\Release /f /s /q
	rmdir $(MAKEDIR)\ApplicationPlugins\InternetExplorer\Release /s /q
	del $(MAKEDIR)\ApplicationPlugins\InternetExplorer\Debug /f /s /q
	rmdir $(MAKEDIR)\ApplicationPlugins\InternetExplorer\Debug /s /q
	del $(MAKEDIR)\install\plugins /f /s /q
	rmdir $(MAKEDIR)\install\plugins /s /q
	del $(MAKEDIR)\install\logs /f /s /q
	rmdir $(MAKEDIR)\install\logs /s /q
	del $(MAKEDIR)\install /f /s /q
	rmdir $(MAKEDIR)\install /s /q
	del $(MAKEDIR)\Debug\log /f /s /q
	rmdir $(MAKEDIR)\Debug\log /s /q
	del $(MAKEDIR)\Debug\logs /f /s /q
	rmdir $(MAKEDIR)\Debug\logs /s /q
	del $(MAKEDIR)\Debug /f /s /q
	rmdir $(MAKEDIR)\Debug /s /q
	del $(MAKEDIR)\Release /f /s /q
	rmdir $(MAKEDIR)\Release /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\Release /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\Release /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\Debug /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\Debug /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\*log /f /s /q

	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\FileMonitor\i386 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\FileMonitor\i386 /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\FileMonitor\objchk_wxp_x86\i386 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\FileMonitor\objchk_wxp_x86\i386 /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\FileMonitor\objfre_wxp_x86\i386 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\FileMonitor\objfre_wxp_x86\i386 /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\FileMonitor\objchk_wxp_x86 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\FileMonitor\objchk_wxp_x86 /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\FileMonitor\objfre_wxp_x86 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\FileMonitor\objfre_wxp_x86 /s /q

	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\ProcessMonitor\i386 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\ProcessMonitor\i386 /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\ProcessMonitor\objchk_wxp_x86\i386 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\ProcessMonitor\objchk_wxp_x86\i386 /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\ProcessMonitor\objfre_wxp_x86\i386 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\ProcessMonitor\objfre_wxp_x86\i386 /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\ProcessMonitor\objchk_wxp_x86 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\ProcessMonitor\objchk_wxp_x86 /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\ProcessMonitor\objfre_wxp_x86 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\ProcessMonitor\objfre_wxp_x86 /s /q

	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\RegistryMonitor\i386 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\RegistryMonitor\i386 /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\RegistryMonitor\objchk_wxp_x86\i386 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\RegistryMonitor\objchk_wxp_x86\i386 /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\RegistryMonitor\objfre_wxp_x86\i386 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\RegistryMonitor\objfre_wxp_x86\i386 /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\RegistryMonitor\objchk_wxp_x86 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\RegistryMonitor\objchk_wxp_x86 /s /q
	del $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\RegistryMonitor\objfre_wxp_x86 /f /s /q
	rmdir $(MAKEDIR)\KernelDrivers\CaptureKernelDrivers\RegistryMonitor\objfre_wxp_x86 /s /q
