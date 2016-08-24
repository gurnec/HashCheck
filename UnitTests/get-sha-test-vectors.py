#!/usr/bin/python3
#
# SHA test vector downloader & builder
# Copyright (C) 2016 Christopher Gurnee.  All rights reserved.
#
# Please refer to readme.md for information about this source code.
# Please refer to license.txt for details about distribution and modification.
#
# Downloads/builds SHA1-3 test vectors from the NIST Cryptographic Algorithm Validation Program

import os, os.path, urllib.request, io, zipfile, glob, re


# Determine and if necessary create the output directory

test_vectors_dir = os.path.join(os.path.dirname(__file__), 'vectors\\')
if not os.path.isdir(test_vectors_dir):
    os.mkdir(test_vectors_dir)


# Download and unzip the two NIST test vector "response" files

for sha_url in ('http://csrc.nist.gov/groups/STM/cavp/documents/shs/shabytetestvectors.zip',
                'http://csrc.nist.gov/groups/STM/cavp/documents/sha3/sha-3bytetestvectors.zip'):
    print('downloading and extracting', sha_url)
    with urllib.request.urlopen(sha_url) as sha_downloading:              # open connection to the download url;
        with io.BytesIO(sha_downloading.read()) as sha_downloaded_zip:    # download entirely into ram;
            with zipfile.ZipFile(sha_downloaded_zip) as sha_zipcontents:  # open the zip file from ram;
                sha_zipcontents.extractall(test_vectors_dir)              # extract the zip file into the output dir


# Convert each response file into a set of test vector files and a single expected .sha* file

print('creating test vector files and expected .sha* files from NIST response files')
rsp_filename_re = re.compile(r'\bSHA([\d_]+)(?:Short|Long)Msg.rsp$', re.IGNORECASE)

for rsp_filename in glob.iglob(test_vectors_dir + '*.rsp'):

    rsp_filename_match = rsp_filename_re.search(rsp_filename)
    if not rsp_filename_match:  # ignore the Monte Carlo simulation files
        continue

    print('    processing', rsp_filename_match.group(0))
    with open(rsp_filename) as rsp_file:

        # Create the expected .sha file which covers this set of test vector files
        with open(rsp_filename + '.sha' + rsp_filename_match.group(1).replace('_', '-'), 'w', encoding='utf8') as sha_file:
            
            dat_filenum = 0
            for line in rsp_file:

                # The "Len" line, specifies the length of the following test vector in bits
                if line.startswith('Len ='):
                    dat_filelen = int(line[5:].strip())
                    dat_filelen, dat_filelenmod = divmod(dat_filelen, 8)
                    if dat_filelenmod != 0:
                        raise ValueError('unexpected bit length encountered (not divisible by 8)')

                # The "Msg" line, specifies the test vector encoded in hex
                elif line.startswith('Msg ='):
                    dat_filename = rsp_filename + '-{:04}.dat'.format(dat_filenum)
                    dat_filenum += 1
                    # Create the test vector file
                    with open(dat_filename, 'wb') as dat_file:
                        dat_file.write(bytes.fromhex(line[5:].strip()[:2*dat_filelen]))
                    del dat_filelen

                # The "MD" line, specifies the expected hash encoded in hex
                elif line.startswith('MD ='):
                    # Write the expected hash to the .sha file which covers this test vector file
                    print(line[4:].strip(), '*' + os.path.basename(dat_filename), file=sha_file)
                    del dat_filename

print("done")
