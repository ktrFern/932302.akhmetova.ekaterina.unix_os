#!/bin/sh -e

File="$1"

if [ -z "$File" ]; then
    echo "Error: no source file specified"
    exit 1
fi

if [ ! -f "$File" ]; then
    echo "Error: file '$File' not found"
    exit 2
fi

Output=$(grep '&Output' "$File" | cut -d: -f2- | xargs)

if [ -z "$Output" ]; then
    echo "Error: no &Output found in file"
    exit 3
fi

TmpDir=$(mktemp -d)
Path=$(pwd)

cleanup() {
    local rc=$?
    rm -rf "$TmpDir"
    exit $rc
}
trap cleanup EXIT HUP INT QUIT PIPE TERM

echo "Building in temporary directory: $TmpDir"

cp "$File" "$TmpDir/"
cd "$TmpDir"

echo "Compiling $File -> $Output"

if g++ "$(basename "$File")" -o "$Output"; then
    mv "$Output" "$Path/"
    echo "Success: output file created at $Path/$Output"
else
    echo "Error: compilation failed"
    exit 4
fi
