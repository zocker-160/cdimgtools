#!/bin/sh
# version.sh - execute in the root of the project to get the version
#   Unlimited permission to copy, distribute and modify this file is granted.
#   This file is offered as-is, without any warranty.

test $# -gt 0 && PACKAGE_NAME=$1
PACKAGE_NAME=${PACKAGE_NAME=cdimgtools}
DEF_VER="unknown_version"

NL='
'

# First try git-describe, then see if there is a VERSION file (included in
# release tarballs), then see if the project directory matches the project
# name, then use the default.
if
	test -d .git || test -f .git &&
	VN=$(git describe --abbrev=7 --match "version/*" --tags HEAD --always 2>/dev/null) &&
	case $VN in
	*$NL*)
		false ;;
	version/*)
		git update-index -q --refresh
		test -z "$(git diff-index --name-only HEAD --)" ||
		VN="$VN.dirty" ;;
	esac
then
	VN=$(echo "$VN" | sed -e 's#^[vV][eE][rR][a-zA-Z]\{0,\}/##' -e 's/-/+/' -e 's/-/_/')
	  # <tag>+<num-of-commits>_g<hash>.dirty
elif
	test -f VERSION && test -s VERSION
then
	VN=$(cat VERSION) || VN=$DEF_VER
elif
	VN=$(pwd | sed -e 's#^.\{0,\}/##') &&
	test x"$VN" != x"${VN#"$PACKAGE_NAME-"}"
then
	VN=${VN#"$PACKAGE_NAME-"}
else
	VN=$DEF_VER
fi

echo "$VN"
