/**
* IsSSD function
* Copyright (C) 2016 Christopher Gurnee.  All rights reserved.
*
* Please refer to readme.txt for information about this source code.
* Please refer to license.txt for details about distribution and modification.
*
* Credit for this code goes to hatenablog.com user NyaRuRu; originally from:
* http://nyaruru.hatenablog.com/entry/2012/09/29/063829
**/

#include "globals.h"
#include "IsSSD.h"
#include "libs/SimpleString.h"
#include <Strsafe.h>

// Tries to determine if the given file is stored on an SSD or other
// device with a fast seek time; errs on the side of not-on-an-SSD
BOOL IsSSD(LPCWCH lpszPath)
{
#ifdef FORCE_PPL
    return(TRUE);
#endif
    TCHAR szMountPoint[MAX_PATH];
    if (! GetVolumePathName(lpszPath, szMountPoint, MAX_PATH))
        return(FALSE);

    TCHAR szVolumeGUID[MAX_PATH];
    if (! GetVolumeNameForVolumeMountPoint(szMountPoint, szVolumeGUID, MAX_PATH))
        return(FALSE);

    // Remove any trailing backslash
    size_t cchVolumeGUIDLen;
    StringCchLength(szVolumeGUID, MAX_PATH, &cchVolumeGUIDLen);
    if (szVolumeGUID[cchVolumeGUIDLen - 1] == '\\')
        szVolumeGUID[cchVolumeGUIDLen - 1] = '\0';

    HANDLE hVolume = CreateFile(
        szVolumeGUID,
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hVolume == INVALID_HANDLE_VALUE)
        return(FALSE);

    // There could be multiple extents on which this path resides; this checks only the first
    VOLUME_DISK_EXTENTS vde;
    DWORD cbIoControlReturned;
    if (! DeviceIoControl(
          hVolume,
          IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
          NULL,
          0,  
          &vde,
          sizeof(vde),
          &cbIoControlReturned,
          NULL))
    {
        CloseHandle(hVolume);
        return(FALSE);
    }

    CloseHandle(hVolume);

    TCHAR szPhysicalDrivePath[MAX_PATH];
    static const TCHAR szPhysicalDrivePrefix[] = TEXT("\\\\.\\PhysicalDrive");
    SSStaticCpy(szPhysicalDrivePath, szPhysicalDrivePrefix);

    StringCchPrintf(
        szPhysicalDrivePath + countof(szPhysicalDrivePrefix) - 1,
        MAX_PATH - countof(szPhysicalDrivePrefix),
        TEXT("%d"),
        vde.Extents[0].DiskNumber);

    HANDLE hPhysicalDrive = CreateFile(
        szPhysicalDrivePath,
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hPhysicalDrive == INVALID_HANDLE_VALUE)
        return FALSE;

    STORAGE_PROPERTY_QUERY spq;
    spq.PropertyId = StorageDeviceSeekPenaltyProperty;
    spq.QueryType  = PropertyStandardQuery;
    DEVICE_SEEK_PENALTY_DESCRIPTOR dspd;
    if (! DeviceIoControl(
          hPhysicalDrive,
          IOCTL_STORAGE_QUERY_PROPERTY,
          &spq,
          sizeof(spq),
          &dspd,
          sizeof(dspd),
          &cbIoControlReturned,
          NULL))
    {
        CloseHandle(hPhysicalDrive);
        return(FALSE);
    }

    CloseHandle(hPhysicalDrive);

    return(dspd.IncursSeekPenalty ? FALSE : TRUE);
}
