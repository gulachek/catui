#!/bin/sh

md() {
	if [ ! -d "$1" ]; then
		mkdir "$1"
	fi
}

untar() {
	local dir
	local file
	local url

	while getopts "u:f:d:" opt; do
		case "$opt" in
			u)
				url="$OPTARG"
				;;
			f)
				file="$OPTARG"
				;;
			d)
				dir="$OPTARG"
				;;
			?)
				echo untar: unknown option \'$opt\'
				return 1
				;;
		esac
	done

	local olddir="$PWD"
	cd "${dir:?untar: Directory not specified with -d}"

	local ec=0

	if [ -n "$url" ]; then
		curl -L "$url" | tar xz --strip-components=1
	elif [ -n "$file" ]; then
		tar xfz "$file" --strip-components=1
	else
		echo "untar: Neither file (-f) nor url (-u) specified"
		ec=1
	fi

	cd "$olddir"
	return $ec
}

VENDOR="$PWD/vendor"
VENDORSRC="$VENDOR/src"

md "$VENDOR"
md "$VENDORSRC"
