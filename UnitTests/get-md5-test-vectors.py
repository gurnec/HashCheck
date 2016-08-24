#!/usr/bin/python3
#
# MD5 & SHA1 test vector downloader
# Copyright (C) 2016 Christopher Gurnee.  All rights reserved.
#
# Please refer to readme.md for information about this source code.
# Please refer to license.txt for details about distribution and modification.
#
# Downloads MD5 and SHA1 test vectors from the NIST National Software Reference Library

import os, os.path, urllib.request, io, zipfile, glob, re

# Determine the script directory
script_dir = os.path.dirname(__file__) or '.'


# Download and unzip the NIST test vectors
# (their files are all in a 'vectors' subdirectory in the zip file)

md5_url = 'http://www.nsrl.nist.gov/testdata/NSRLvectors.zip'
print('downloading and extracting', md5_url)
with urllib.request.urlopen(md5_url) as md5_downloading:              # open connection to the download url;
    with io.BytesIO(md5_downloading.read()) as md5_downloaded_zip:    # download entirely into ram;
        with zipfile.ZipFile(md5_downloaded_zip) as md5_zipcontents:  # open the zip file from ram;
            md5_zipcontents.extractall(script_dir)                    # extract the zip file into the script dir


# Convert the two non-standard NIST hash files into the expected .sha1/.md5 files

print('creating expected .sha1/.md5 files from NIST files')
nist_filename_re = re.compile(r'\bbyte-hashes\.(\w+)$', re.IGNORECASE)
hash_re          = re.compile(r'^([\da-f]{32}(?:[\da-f]{8})?) \^$', re.IGNORECASE)

for nist_filename in glob.iglob(script_dir + r'\vectors\byte-hashes.*'):

    nist_filename_match = nist_filename_re.search(nist_filename)
    if not nist_filename_match:
        raise RuntimeError('unexpected hash filename ' + nist_filename);

    print("    processing", nist_filename_match.group(0));
    with open(nist_filename) as nist_file:

        # Create the expected .sha1/.md5 file which covers the hashes in this nist file
        with open(script_dir + r'\vectors\hashcheck.' + nist_filename_match.group(1), 'w', encoding='utf8') as hashcheck_file:

            dat_filenum = 0
            for line in nist_file:

                # If this is line is a valid hash line, write it (in the converted format) to the .sha1/.md5 file
                hash_match = hash_re.match(line)
                if hash_match:
                    print(hash_match.group(1), '*byte{:04}.dat'.format(dat_filenum), file=hashcheck_file)
                    dat_filenum += 1

print("done")
