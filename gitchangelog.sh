#!/bin/sh
# gitchangelog.sh - portable script generating a GNU-like changelog from a Git log
# Copyright © 2013 Géraud Meyer <graud@gmx.com>
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License version 2 as
#   published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
#   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
#   for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program.  If not, see <http://www.gnu.org/licenses/>.

PROGRAM_NAME="gitchangelog.sh"
PROGRAM_VERSION="1.1"
# Usage:
#   gitchangelog.sh [<options>] [--] [ {-|<outfile>} [ {-|<git_log_args>} ] ]

# parameters
GITBODY= NO_GITBODY=yes
BLANKLINE=yes
TAGS=
TAG_PATTERN='[^\n]\+'
MERGE=
TITLE= NO_TITLE=yes
while [ $# -gt 0 ]
do
	case $1 in
	--tags)
		TAGS=yes ;;
	--tag-pattern)
		shift; TAG_PATTERN=${1-} ;;
	--merge)
		MERGE=yes ;;
	--title-only)
		TITLE=yes NO_TITLE= ;;
	--git-body)
		GITBODY=yes NO_GITBODY= ;;
	--no-blankline)
		BLANKLINE= ;;
	--version)
		echo "$PROGRAM_NAME version $PROGRAM_VERSION"
		exit ;;
	--?*)
		echo "$0: unknown option $1" >&2
		exit 255 ;;
	--)
		shift; break ;;
	*)
		break ;;
	esac
	shift
done
# git repository check
test x"${2-}" != x"-" &&
test ! -d .git && ! git rev-parse --git-dir >/dev/null &&
{
	echo "$0: error not in a git repository" >&2
	exit 255
}
# output file
test $# -gt 0 &&
{
	test x"${1-}" != x"-" &&
	exec >"$1"
	shift
}

# input source
B='{' b='}'
s='\'
  # some shells, like the family of pdksh, behave differently regarding
  # the backslash in variable substitutions inside a here document
if test x"${1-}" = x"-"
then cat
else git log --date=short ${TAGS:+--decorate} ${1+"$@"}
fi |
# processing
LC_ALL=C sed -e "$(# Transform the GNU sed program into a more portable one
  LC_ALL=C sed -e 's/\\s/[ \\t]/g
    s/\\+/\\{1,\\}/g
    s/; /\
/g
    s/@n/\\\
/g
    s/\\t/	/g' <<EOF
  # Put the tags in the hold space
  /^commit / {
    s/^commit [a-zA-Z0-9]\+//
    s/^ \+(\(.*\))\$/, \1/; s/, /@n/g
    # conditionnal branch to reset
    s/^/@n/; t a
    :a; s/\n\$//; t e
        s/\(.*\)\ntag: \($TAG_PATTERN\)\$/[\2]@n\1/; t a
        s/\(.*\)\n.*\$/\1/; t a
    :e; s/\n/ /g; h; d
  }
  # Add the merge marker to the hold space
  /^Merge: / { ${MERGE:+x; s/\$/${B}M$b /; x;} d; }
  /^Author:/ {
    # Process author, date, tag
    ${TAGS:+x; /^\$/! $B s/${s}s*\$//; s/\$/@n/; $b; x; b s;} x; s/.*//; x ${TAGS:+; :s}
    N; G; s/^Author:\s*\([^\n]*\) \s*\(<[^ \n]*>\)\s*\nDate:\s*\([^\n]*\)\n\(.*\)/\4\3  \1  \2@n/
    # Process title
    n; N; s/[^\n]*\n    /\t${TITLE:+${NO_GITBODY:+* }}/
    # If non empty body, print an extra line
    n; ${NO_TITLE:+${NO_GITBODY:+${BLANKLINE:+/^\$/! $B s/^    /${s}t/p; s/^${s}t/    /; $b}}}
  }
  ${TITLE:+/^    / d}
  # First line of the paragraph
  :b; /^    \$/ { N; s/^    \n//; s/^    \(.\)/${GITBODY:+${s}t@n}\t${NO_GITBODY:+* }\1/; b b; }
  # Rest of the paragraph
  s/^    /\t${NO_GITBODY:+  }/
  # Reset the hold space for the next entry
  /^\$/ h
EOF
)"
rc=$?
# error check
test $rc -eq 0 ||
echo "$0: ERROR sed failed with #$rc" >&2
exit $rc
