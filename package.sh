#!/bin/bash
# Bumps the minor version in CMakeLists.txt, builds, runs the tests, and writes
# ../blitzpad-<major>.<minor>.zip (source, build.sh, and a prebuilt binary).
# Usage: ./package.sh [build-dir]
set -e
cd "$(dirname "$0")"
BUILD="${1:-build}"

old=$(sed -n 's/^project(blitzpad VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
IFS=. read -r major minor patch <<<"$old"
new="$major.$((minor + 1)).0"
sed -i "s/^project(blitzpad VERSION $old/project(blitzpad VERSION $new/" CMakeLists.txt
echo "Version $old -> $new"

cmake -S . -B "$BUILD" >/dev/null
cmake --build "$BUILD" -j"$(nproc)"
ctest --test-dir "$BUILD" --output-on-failure

stage=$(mktemp -d)
# Files sit at the top of the zip (no wrapping folder), so unzipping into a
# git repository puts README.md, CHANGELOG.md, docs/ and data/ at the repo
# root, where GitHub expects them.
mkdir -p "$stage/prebuilt"
cp -r CMakeLists.txt README.md CHANGELOG.md LICENSE.md MANIFEST.md build.sh install.sh deps.sh package.sh .gitignore data docs packaging src tests tools "$stage/"
cp "$BUILD/blitzpad" "$stage/prebuilt/" && strip "$stage/prebuilt/blitzpad"
chmod +x "$stage/build.sh" "$stage/install.sh" "$stage/deps.sh" "$stage/package.sh" "$stage/packaging/build-deb.sh" "$stage/packaging/build-rpm.sh" "$stage/prebuilt/blitzpad"
zip="$(cd .. && pwd)/blitzpad-$major.$((minor + 1)).zip"
rm -f "$zip"
(cd "$stage" && python3 -c "import shutil,sys; shutil.make_archive(sys.argv[1][:-4], 'zip', '.', '.')" "$zip")
rm -rf "$stage"
echo "Wrote $zip"
