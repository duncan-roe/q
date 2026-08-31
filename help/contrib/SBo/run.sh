#!/bin/sh

# n205#1
q -oi'fb^Ju,^ND^N^<4003>/contrib/SBo/SBo.qm^J^ND^N^<202>l n205#1:^J^NC^N\^[l#n205,5,,,1^J^NC^N\^D^Jb^J^ND^N^<220>q$1^J^ND^N^<205>^ND^N^<202>!mv ^ND^N^<4002>.bu ^ND^N^<4002>^J' $(find . -name "*.SlackBuild")

# Pending action by maintainer or me
git checkout -- academic/Kst/Kst.SlackBuild

git commit -a -m"n205#1: Get rid of unused LIBDIRSUFFIX and SLKCFLAGS lines"

# n205#2
q -oi'fb^Ju,^ND^N^<4003>/contrib/SBo/SBo.qm^J^ND^N^<202>l n205#2:^J^NC^N\^[l#n205,1,,,1^J^NC^N\^D^Jb^J^ND^N^<220>^ND^N^<205>^ND^N^<202>!mv ^ND^N^<4002>.bu ^ND^N^<4002>^J' $(find . -name "*.SlackBuild")
git commit -a -m"n205#2: Fix most instances of \"suggest replacing 'python' with 'python2' ...\""

# n205#3
q -oi'fb^Ju,^ND^N^<4003>/contrib/SBo/SBo.qm^J^ND^N^<202>l n205#3:^J^NC^N\^[l#n205,1,,,1^J^NC^N\^D^Jb^J^ND^N^<220>q$1^J^ND^N^<205>^ND^N^<202>!mv ^ND^N^<4002>.bu ^ND^N^<4002>^J' \
  libraries/tlsh/tlsh.SlackBuild  network/pydio-sync/pydio-sync.SlackBuild network/sqlmap/sqlmap.SlackBuild python/pyfltk/pyfltk.SlackBuild python/simplejson/simplejson.SlackBuild system/autojump/autojump.SlackBuild
git commit -a -m"n205#3: Fix remaining instances"

# n205#4
cat sbolint.log | grep -E 'canonical pypi URL should be https' > foo
cp foo bar
q -oi'fb^Ju,^ND^N^<4003>/contrib/SBo/SBo.qm^J^ND^N4a^J^J^[e foo^Js^J' bar
q -oi'u,^ND^N^<4003>/contrib/SBo/SBo.qm^J^ND^N^<202>l n205#4:^J^NC^N\^[l#n205,,,,1^J^NC^N\^D^Jb^J^ND^N^<220>q$1^J^ND^N^<205>^ND^N^<202>!mv ^ND^N^<4002>.bu ^ND^N^<4002>^J' bar
git commit -a -m"n205#4: Fix \"canonical pypi URL should be ...\" warnings"
