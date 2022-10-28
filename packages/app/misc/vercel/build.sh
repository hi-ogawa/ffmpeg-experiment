#!/bin/bash
set -eu -o pipefail

this_dir="$(dirname "$(readlink -f "$0")")"
app_dir="$(dirname "$(dirname "$this_dir")")"
output_dir="$app_dir/.vercel/output"

rm -rf "$output_dir"
mkdir -p "$output_dir"

cp "$this_dir/config.json" "$output_dir"
cp -r "$app_dir/build" "$output_dir/static"
