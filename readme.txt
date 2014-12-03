HashCheck Shell Extension
Copyright (C) Kai Liu.  All rights reserved.


I.  Building the source

    1.  How the "official" binaries were built

        Environment: Visual Studio 6.0 SP6
        Compiler for x86-32: Version 13.1 (from the Windows Server 2003 SP1 DDK)
        Compiler for x86-64: Version 14.0 (from the Windows Server 2003 R2 PSDK)
        Platform SDK: 5.2.3790.2075 (Windows Server 2003 R2 Platform SDK)
        Resource Compiler: Version 6.1.6723.1

    2.  Building 32-bit binaries with Microsoft Visual Studio

        a.  Visual Studio 6 - SUPPORTED

            Prerequisites:
            * Update Visual Studio to SP5 or SP6
            * Update the PSDK (see "Notes")
            * Update the resource compiler (see the "Resources" section below)

            Notes:
            * Use the HashCheck.dsp project file
            * The August 2004 (XPSP2) Platform SDK is the last PSDK to be fully-
              compatible with VC6; you may use a newer PSDK for 64-bit compiler
              support, but the automatic integration will not work with VC6.

        b.  Visual Studio 7-8 (2002/2003/2005) - SUPPORTED

            Prerequisites:
            * Update the resource compiler (see the "Resources" section below)

            Notes:
            * Use the HashCheck.vcproj project file

        c.  Visual Studio 9-10 (2008/2010) - SUPPORTED

            Prerequisites:
            * None

            Notes:
            * Use the HashCheck.vcproj project file

    3.  Building 64-bit binaries with Microsoft Visual Studio

        a.  Visual Studio 6 - SUPPORTED

            Prerequisites:
            * Update Visual Studio to SP6
            * Update the PSDK to either 5.2.3790.1830 or 5.2.3790.2075
            * Update the resource compiler (see the "Resources" section below)

            Notes:
            * Use the HashCheck.dsp project file
            * To use the PSDK's 64-bit compiler with VC6, open a PSDK command
              prompt with the environment set to 64-bit, and then start up
              VC6 from that command prompt using the /USEENV switch.

        b.  Visual Studio 7 (2002/2003) - UNSUPPORTED

        c.  Visual Studio 8 (2005) - SUPPORTED

            Prerequisites:
            * Update the resource compiler (see the "Resources" section below)

            Notes:
            * Use the HashCheck.vcproj project file
            * The Express edition does not support 64-bit compilation.

        d.  Visual Studio 9-10 (2008/2010) - SUPPORTED

            Prerequisites:
            * None

            Notes:
            * Use the HashCheck.vcproj project file
            * The Express edition does not support 64-bit compilation.

    4.  Building under other environments

        I have not tested this source with other Windows compilers, so I do not
        know how well it will compile outside of Microsoft Visual Studio.

    5.  Resources

        The resource script (HashCheck.rc) should be compiled with version 6 or
        newer of Microsoft's Resource Compiler.  Unfortunately, Visual Studio
        2005 and earlier only include version 5 of the Resource Compiler; users
        of Visual Studio 2008 or newer do not need to worry about this.

        You can get the latest version of the Resource Compiler by installing
        the Vista SDK (or any newer SDK), which can be downloaded for free from
        Microsoft.  If you want a smaller download, you can get version
        6.1.6723.1 of the Resource Compiler by downloading KB949408 [1], and
        then use 7-Zip 4.52 or newer to unpack the installer, then the .msp
        found inside, and then the CAB found inside of that.

        If you want to compile with version 5 of the Resource Compiler, you can
        do so by converting HashCheck.rc and HashCheckTranslations.rc from UTF-8 to
        UTF-16 (double-byte little-endian Unicode) and commenting out this line:
        #pragma code_page(65001)

        More information about this issue can be found in the comments of the
        resource script.

        [1] http://www.microsoft.com/downloads/details.aspx?FamilyID=53f9cbb4-b4af-4cf2-bfe5-260cfb90f7c3

    6.  Localizations

        Translation strings are stored as string table resources.  These tables
        can be modified by editing HashCheckTranslations.rc.


II. License and miscellanea

    Please refer to license.txt for details about distribution and modification.

    If you would like to report a bug, make a suggestion, or contribute a
    translation, you can do so at the following URL:
    http://code.kliu.org/tracker/

    The latest version of this software can be found at:
    http://code.kliu.org/hashcheck/
