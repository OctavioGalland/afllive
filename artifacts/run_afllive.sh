#!/bin/bash

artifacts_folder=$(dirname $0)
testamp_subjects="libxml2 openssl opus"

print_usage () {
    echo "Usage: $0 [project] [generated|curated] [output_path] [campaign duration in seconds]"
    echo "Available projects:"
    echo "    boringssl, bzip2 (curated only), htslib, leptonica, libaom, libass, libexif (curated only), libgsm (curated only), libvpx, libxml2, openssl, opus"
}

copy_repo_to_directory () {
    mkdir $1/afllive
    for file in $(ls $artifacts_folder/.. | grep -v artifacts)
    do
        cp -rf $artifacts_folder/../$file $1/afllive/
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
    cp "$artifacts_folder/afllive/$subject/auto_config.json" "$artifacts_folder/afllive/$subject/config.json"
    cp "$artifacts_folder/afllive/$subject/auto_config.json" "$artifacts_folder/afllive/$subject/coverage/config.json"
elif [ "$config" == "curated" ]
then
    cp "$artifacts_folder/afllive/$subject/curated_config.json" "$artifacts_folder/afllive/$subject/config.json"
    cp "$artifacts_folder/afllive/$subject/curated_config.json" "$artifacts_folder/afllive/$subject/coverage/config.json"
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

mkdir -p "$output_folder"
rm -rf "$output_folder/out" "$output_folder/coverage" "$output_folder/coverage_${subject}_0.txt"

./prepare_host_for_fuzzing.sh

# build fuzzing image
fuzzing_image_name="afllive_$subject"
# backup dockerfile
cp "$artifacts_folder/afllive/$subject/Dockerfile" "$artifacts_folder/afllive/$subject/Dockerfile.backup"
# replace campaign duration with user-provided one
sed -i "s/\/60\//\/$seconds\//g" "$artifacts_folder/afllive/$subject/Dockerfile"
sed -i "s/\"60\"/\"$seconds\"/g" "$artifacts_folder/afllive/$subject/Dockerfile"
docker rmi "$fuzzing_image_name"
copy_repo_to_directory "$artifacts_folder/afllive/$subject"
(cd $artifacts_folder/afllive/$subject && docker build -t "$fuzzing_image_name" .)
rm -rf $artifacts_folder/afllive/$subject/afllive
# restore old dockerfile
mv "$artifacts_folder/afllive/$subject/Dockerfile.backup" "$artifacts_folder/afllive/$subject/Dockerfile"
 
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
cp -rf "$artifacts_folder/afllive/$subject/coverage" "$output_folder/"
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
rm "$artifacts_folder/afllive/$subject/config.json" "$artifacts_folder/afllive/$subject/coverage/config.json"

echo "Done!"
echo "Coverage file at $output_folder/coverage_${subject}_0.txt"
echo "Crashes at $(find $output_folder -type d -name crashes)"
