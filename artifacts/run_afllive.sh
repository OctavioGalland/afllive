#!/bin/bash

testamp_subjects="libxml2 openssl opus"

print_usage () {
    echo "Usage: $0 [project] [generated|curated] [output_path] [campaign duration in seconds]"
    echo "Available projects:"
    echo "    boringssl, bzip2 (curated only), htslib, leptonica, libaom, libass, libexif (curated only), libgsm (curated only), libvpx, libxml2, openssl, opus"
}

copy_repo_to_directory () {
    mkdir $1/afllive
    for file in $(ls .. | grep -v $(basename $(pwd)))
    do
        cp -rf ../$file $1/afllive/
    done
    rm $(find $1/afllive -name "*.o" -o -name "*.so" -o -name "*.a" -o -name "afl-clang-fast" -o -name "afl-clang-fast++" -o -name "afl-fuzz")
}

if [ "$#" != "4" ]
then
    echo "Wrong number of arguments"
    print_usage
    exit 1
fi

subject=$1
config=$2
output_folder=$3
seconds=$4

if [ "$config" == "generated" ]
then
    cp "afllive/$subject/auto_config.json" "afllive/$subject/config.json"
    cp "afllive/$subject/auto_config.json" "afllive/$subject/coverage/config.json"
elif [ "$config" == "curated" ]
then
    cp "afllive/$subject/curated_config.json" "afllive/$subject/config.json"
    cp "afllive/$subject/curated_config.json" "afllive/$subject/coverage/config.json"
else
    echo "Unknown argument: $config"
    print_usage
    exit 1
fi

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
fuzzing_image_name="afllive_$subject"
# backup dockerfile
cp "afllive/$subject/Dockerfile" "afllive/$subject/Dockerfile.backup"
# replace campaign duration with user-provided one
sed -i "s/\/60\//\/$seconds\//g" "afllive/$subject/Dockerfile"
sed -i "s/\"60\"/\"$seconds\"/g" "afllive/$subject/Dockerfile"
docker rmi "$fuzzing_image_name"
copy_repo_to_directory afllive/$subject
(cd afllive/$subject && docker build -t "$fuzzing_image_name" .)
rm -rf afllive/$subject/afllive
# restore old dockerfile
mv "afllive/$subject/Dockerfile.backup" "afllive/$subject/Dockerfile"
 
# run fuzzing campaign
fuzzing_container_name="afllive_$subject_$(tr -dc a-z </dev/urandom | head -c 8)"
docker run --name "$fuzzing_container_name" "$fuzzing_image_name"

echo "Fuzzing campaign done, collecting coverage"

# retrieve results and clean container
# fuzzer output path is /opt/fuzzing for testamp subjects, /opt/out for all others
fuzzer_output_path="/opt/$( (echo "$testamp_subjects" | grep -q $subject) && echo "fuzzing" || echo "out")"
docker cp "$fuzzing_container_name":"$fuzzer_output_path"  "$output_folder/out"
docker rm "$fuzzing_container_name"

# build coverage image
coverage_image_name="${fuzzing_image_name}_coverage"
cp -rf "afllive/$subject/coverage" "$output_folder/"
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

# clean up config files
rm "afllive/$subject/config.json" "afllive/$subject/coverage/config.json"

echo "Done!"
echo "Coverage file at $output_folder/coverage_${subject}_0.txt"
echo "Crashes at $(find $output_folder -type d -name crashes)"
