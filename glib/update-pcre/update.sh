#! /bin/sh

IN="../update-pcre"
PCRE=$1

if [ "x$PCRE" = x -o "x$PCRE" = x--help -o "x$PCRE" = x-h ]; then
    cat >&2 << EOF

$0 PCRE-DIR

  Updates the local PCRE copy with a different version of the library,
  contained in the directory PCRE-DIR.

  This will delete the content of the local pcre directory, copy the
  necessary files from PCRE-DIR, and generate other needed files, such
  as Makefile.am
EOF
    exit
fi

if [ ! -f gregex.h ]; then
    echo "This script should be executed from the directory containing gregex.c." 2> /dev/null
    exit 1
fi

if [ ! -f $PCRE/Makefile.in -o ! -f $PCRE/pcre_compile.c ]; then
    echo "'$PCRE' does not contain a valid PCRE version." 2> /dev/null
    exit 1
fi


echo "Deleting old PCRE library"
mv pcre/.svn tmp-pcre-svn
rm -R pcre 2> /dev/null
mkdir pcre
cd pcre

# pcre_chartables.c is generated by dfatables.
# We do not want to compile and execute dfatables.c every time, because
# this could be a problem (e.g. when cross-compiling), so now generate
# the file and then distribuite it with GRegex.
echo "Generating pcre_chartables.c"
cp -R $PCRE tmp-build
cd tmp-build
./configure --enable-utf8 --enable-unicode-properties --disable-cpp > /dev/null
make pcre_chartables.c > /dev/null
cat > ../pcre_chartables.c << \EOF
/* This file is autogenerated by ../update-pcre/update.sh during
 * the update of the local copy of PCRE.
 */
EOF
cat pcre_chartables.c >> ../pcre_chartables.c
cd ..
rm -R tmp-build

# Compiled C files.
echo "Generating makefiles"
all_files=`awk '/^OBJ = /, /^\\s*$/ \
            { \
                sub("^OBJ = ", ""); \
                sub(".@OBJEXT@[[:blank:]]*\\\\\\\\", ""); \
                sub("\\\\$\\\\(POSIX_OBJ\\\\)", ""); \
                print; \
            }' \
            $PCRE/Makefile.in`

# Headers.
included_files="pcre.h pcre_internal.h ucp.h ucpinternal.h"

# Generate Makefile.am.
cat $IN/Makefile.am-1 > Makefile.am
for name in $all_files; do
    echo "	$name.c \\" >> Makefile.am
    if [ $name != pcre_chartables ]; then
        # pcre_chartables.c is a generated file.
        cp $PCRE/$name.c .
    fi
done
for f in $included_files; do
    echo "	$f \\" >> Makefile.am
    cp $PCRE/$f .
done
cat $IN/Makefile.am-2 >> Makefile.am

# Generate makefile.msc
cat > makefile.msc << EOF
TOP = ..\..\..
!INCLUDE ..\..\build\win32\make.msc

INCLUDES = \\
	-I ..\.. \\
	-I ..
	
DEFINES = \\
	-DPCRE_STATIC \\
	-DHAVE_CONFIG_H \\
	-DHAVE_LONG_LONG_FORMAT \\
	-DSUPPORT_UCP \\
	-DSUPPORT_UTF8 \\
	-DNEWLINE=-1 \\
	-DMATCH_LIMIT=10000000 \\
	-DMATCH_LIMIT_RECURSION=10000000 \\
	-DMAX_NAME_SIZE=32 \\
	-DMAX_NAME_COUNT=10000 \\
	-DMAX_DUPLENGTH=30000 \\
	-DLINK_SIZE=2 \\
	-DEBCDIC=0 \\
	-DPOSIX_MALLOC_THRESHOLD=10

OBJECTS = \\
`
for f in $all_files; do
    echo "	$f.obj \\\\"
done
`

pcre.lib : \$(OBJECTS)
	lib -out:pcre.lib \$(OBJECTS)

.c.obj:
	\$(CC) \$(CRT) \$(CFLAGS) -Ox -GD -c $<
EOF

echo "Patching PCRE"

# Copy the license.
cp $PCRE/COPYING .

# Use glib for memory allocation.
patch > /dev/null < $IN/memory.patch

# No dllimport/dllexport of pcre symbols
patch > /dev/null <$IN/notdll.patch

# Copy the modified version of pcre_valid_utf8.c.
cp $IN/pcre_valid_utf8.c .

# Copy the modified version of pcre_ucp_searchfuncs.c that uses glib
# for Unicode properties.
cp $IN/pcre_ucp_searchfuncs.c .
patch > /dev/null < $IN/ucp.patch

# Remove the digitab array in pcre_compile.c.
patch > /dev/null < $IN/digitab.patch
sed -i -e 's/(digitab\[\(.*\)\] & ctype_digit)/g_ascii_isdigit(\1)/' pcre_compile.c
sed -i -e 's/(digitab\[\(.*\)\] & ctype_xdigit)/g_ascii_isxdigit(\1)/' pcre_compile.c

# Reduce the number of relocations.
$IN/make_utt.py
patch > /dev/null < $IN/utt.patch
patch > /dev/null < $IN/table-reduction.patch

# Copy back the old SVN directory.
mv ../tmp-pcre-svn .svn


cat << EOF

Update completed. You now should check that everything is working.
Remember to update the regex syntax doc with the new features
(docs/reference/glib/regex-syntax.sgml) and to run the tests.
EOF

