#!/bin/bash
##############################################################################
#  This script downloads the works of Shakespeare and then duplicates them
#  repeatedly in "/books". The resulting directory is 1.3G.
##############################################################################

set -e
set -x

mkdir -p /books
cd /books

wget http://homepages.ihug.co.nz/~leonov/shakespeare.tar.bz2
tar xvf shakespeare.tar.bz2
rm shakespeare.tar.bz2

function double() {
    local old_dir="${1}"
    local new_dir="${2}"
    mkdir "${new_dir}"
    mv "${old_dir}" "${new_dir}"/a
    cp -rf "${new_dir}"/a "${new_dir}"/b
}

double Shakespeare 1
last_number=1
for i in `seq 2 8`; do
    double "${last_number}" "${i}"
    last_number="${i}"
done

echo 'Total size:'
du -sh /books

