#!/usr/bin/python3
#
# Version updater
# Copyright (C) 2016 Christopher Gurnee.  All rights reserved.
#
# Please refer to readme.md for information about this source code.
# Please refer to license.txt for details about distribution and modification.
#
# Updates various version constants based on HASHCHECK_VERSION_STR in version.h

import sys, os, os.path, re, contextlib, atexit
from warnings import warn

atexit.register(lambda: input('Press Enter to exit ...'))


# When used in a 'with' statement, renames filename to filename.orig and opens filename for
# writing. If an uncaught exception is raised, restores filename.orig, otherwise deletes it.
@contextlib.contextmanager
def overwrite(filename, mode='w', *args, **kwargs):
    try:
        renamed = filename + '.orig'
        os.rename(filename, renamed)
    except FileNotFoundError:
        renamed = None
    try:
        file = open(filename, mode, *args, **kwargs)
    except:
        if renamed:
            os.rename(renamed, filename)
        raise
    try:
        yield file
    except:
        file.close()
        os.remove(filename)
        if renamed:
            os.rename(renamed, filename)
        raise
    file.close()
    if renamed:
        os.remove(renamed)


os.chdir(os.path.dirname(__file__))

# Get the "authoritative" version string from HASHCHECK_VERSION_STR in version.h
match = None
with open('version.h', encoding='utf-8') as file:
    for line in file:
        match = re.match(r'#define\s+HASHCHECK_VERSION_STR\s+"(\d+)\.(\d+)\.(\d+)\.(\d+)((?:-\w+)?)"', line)
        if match:
            break
if not match:
    sys.exit('Valid version not found in version.h')

major      = match.group(1)
minor      = match.group(2)
patch      = match.group(3)
build      = match.group(4)
prerelease = match.group(5)
def full_version():
    return '.'.join((major, minor, patch, build)) + prerelease

print('v' + full_version())

# Compare the authoritative version with the one in appveyor.yml; since this file
# is updated last, it will be the same iff the authoritative version wasn't changed
match = None
with open('appveyor.yml', encoding='utf-8') as file:
    for line in file:
        match = re.match(r'version:\s*(\S+)\s*$', line)
        if match:
            if match.group(1) == full_version():
                if input('Version is unchanged, increment build number (Y/n)? ').strip().lower() == 'n':
                    sys.exit(0)
                build = str(int(build) + 1)
                print('v' + full_version())
            break

# Update the 3 version constants in version.h
found_version_full   = 0
found_version_str    = 0
found_linker_version = 0
with overwrite('version.h', encoding='utf-8', newline='') as out_file:
    with open('version.h.orig', encoding='utf-8', newline='') as in_file:
        for line in in_file:
            (line, subs) = re.subn(r'^#define\s+HASHCHECK_VERSION_FULL\s+[\d,]+',
                                     '#define HASHCHECK_VERSION_FULL ' + ','.join((major, minor, patch, build)), line)
            found_version_full   += subs
            (line, subs) = re.subn(r'^#define\s+HASHCHECK_VERSION_STR\s+"[\d.\w-]*"',
                                     '#define HASHCHECK_VERSION_STR "' + full_version() + '"', line)
            found_version_str    += subs
            (line, subs) = re.subn(r'^#pragma\s+comment\s*\(\s*linker\s*,\s*"/version:[\d+.]+"\s*\)',
                                     '#pragma comment(linker, "/version:{}.{}")'.format(major, minor), line)
            found_linker_version += subs
            out_file.write(line)
if found_version_full   != 1:
    warn('found {} HASHCHECK_VERSION_FULL defines in version.h'.format(found_version_full))
if found_version_str    != 1:
    warn('found {} HASHCHECK_VERSION_STR defines in version.h'.format(found_version_str))
if found_linker_version != 1:
    warn('found {} linker /version lines in version.h'.format(found_linker_version))

# Update the 4 version constants in HashCheck.nsi
found_outfile             = 0
found_product_version     = 0
found_version_key_product = 0
found_version_key_file    = 0
with overwrite(r'installer\HashCheck.nsi', encoding='utf-8', newline='') as out_file:
    with open(r'installer\HashCheck.nsi.orig', encoding='utf-8', newline='') as in_file:
        for line in in_file:
            (line, subs) = re.subn(r'^OutFile\s*"HashCheckSetup-v[\d.\w-]+.exe"',
                                     'OutFile "HashCheckSetup-v' + full_version() + '.exe"', line)
            found_outfile             += subs
            (line, subs) = re.subn(r'^VIProductVersion\s+"[\d.\w-]+"',
                                     'VIProductVersion "' + full_version() + '"', line)
            found_product_version     += subs
            (line, subs) = re.subn(r'^VIAddVersionKey\s+/LANG=\${LANG_ENGLISH}\s+"ProductVersion"\s+"[\d.\w-]+"',
                                     'VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "' + full_version() + '"', line)
            found_version_key_product += subs
            (line, subs) = re.subn(r'VIAddVersionKey\s+/LANG=\${LANG_ENGLISH}\s+"FileVersion"\s+"[\d.\w-]+"',
                                    'VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "' + full_version() + '"', line)
            found_version_key_file    += subs
            out_file.write(line)
if found_outfile             != 1:
    warn('found {} OutFile statements in HashCheck.nsi'.format(found_outfile))
if found_product_version     != 1:
    warn('found {} VIProductVersion\'s in HashCheck.nsi'.format(found_product_version))
if found_version_key_product != 1:
    warn('found {} ProductVersion VIAddVersionKeys defines in HashCheck.nsi'.format(found_version_key_product))
if found_version_key_file    != 1:
    warn('found {} FileVersion VIAddVersionKeys defines in HashCheck.nsi'.format(found_version_key_file))

# Lastly, update the one version line in appveyor
found_version = 0
with overwrite('appveyor.yml', encoding='utf-8', newline='') as out_file:
    with open('appveyor.yml.orig', encoding='utf-8', newline='') as in_file:
        for line in in_file:
            (line, subs) = re.subn(r'^version:\s*\S+', 'version: ' + full_version(), line)
            found_version += subs
            out_file.write(line)
if found_version != 1:
    warn('found {} version lines in appveyor.yml'.format(found_version))

print('Done.')
