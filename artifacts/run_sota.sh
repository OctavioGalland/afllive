#!/bin/bash

artifacts_folder=$(dirname $0)

print_usage () {
    echo "Usage: $0 [project] [output_path] [campaign duration in seconds]"
    echo "Available projects:"
    echo "    htslib leptonica libaom libgsm libvpx"
}

copy_repo_to_directory () {
    mkdir $1/afllive
    for file in $(ls $artifacts_folder/.. | grep -v artifacts)
    do
        cp -rf $artifacts_folder/../$file $1/afllive/
    done
    rm $(find $1/afllive -name "*.o" -o -name "*.so" -o -name "*.a" -o -name "afl-clang-fast" -o -name "afl-clang-fast++" -o -name "afl-fuzz")
}

if [ "$#" != "3" ]
then
    echo "Wrong number of arguments"
    print_usage
    exit 1
fi

subject=$1
output_folder=$2
seconds=$3

# output_folder needs to be absolute
if [ "$output_folder" == "${output_folder#/}" ]
then
    output_folder="$(pwd)/$output_folder"
fi

# check if output folder is empty
mkdir -p "$output_folder"

if [ "$(ls $output_folder | wc -l)" != "0" ]
then
    echo "Output folder '$output_folder' exists and is not empty, abroting"
    exit 1
fi

./prepare_host_for_fuzzing.sh

# build fuzzing image
fuzzing_image_name="sota_$subject"
# backup dockerfile
cp "$artifacts_folder/sota/$subject/run.sh" "$artifacts_folder/sota/$subject/run.sh.backup"
# replace campaign duration with user-provided one
sed -i "s/=60/=$seconds/g" "$artifacts_folder/sota/$subject/run.sh"
docker rmi "$fuzzing_image_name"
(cd $artifacts_folder/sota/$subject && docker build -t "$fuzzing_image_name" .)
# restore old dockerfile
mv "$artifacts_folder/sota/$subject/run.sh.backup" "$artifacts_folder/sota/$subject/run.sh"

# run fuzzing campaign
fuzzing_container_name="sota_$subject_$(tr -dc a-z </dev/urandom | head -c 8)"
docker run --name "$fuzzing_container_name" "$fuzzing_image_name"

echo "Fuzzing campaign done, collecting coverage"

# retrieve results and clean container
fuzzer_output_path="/opt/out"
docker cp "$fuzzing_container_name":"$fuzzer_output_path"  "$output_folder/out"
docker rm "$fuzzing_container_name"

# build coverage image
coverage_image_name="${fuzzing_image_name}_coverage"
cp -rf "$artifacts_folder/sota/$subject/coverage" "$output_folder/"
docker rmi "$coverage_image_name"
copy_repo_to_directory $output_folder/coverage
(cd "$output_folder/coverage" && docker build -t "$coverage_image_name" .)
rm -rf $output_folder/coverage/afllive

# run coverage
coverage_container_name="${fuzzing_container_name}_coverage"
docker run --name "$coverage_container_name" -v "$output_folder/out":/opt/corpuses/out_0 "$coverage_image_name"

# retrieve results and clean container
docker cp "$coverage_container_name":"/opt/coverage_${subject}_0.txt" "$output_folder"
docker rm "$coverage_container_name"

echo "Done!"
echo "Coverage file at $output_folder/coverage_${subject}_0.txt"
echo "Crashes at $(find $output_folder -type d -name crashes)"

