fd y
fb
fm-s -l
t1 81
N A '^NC^NS^N\o off^J^a^J^I^I^NR2^NOB^[^<PS0>^<PSHTAB 1>m1,32000^N^<701> Fit into TAB2-1
N700 ^X^NA^N^<701>^NG ^N^<701>^N^<702> ;Leave lines starting "> " alone
N701 ^J^NC^NU^NA^N^<706>^NG>^N^<700>^N^<702> ;Leave lines starting "> " alone
N E 'T1 81^J^N^A^NU ;Editing width
N J '^G^*^S^F^Q^NU ;control char in HELP file u/c
N I '^NC^NS^N\G1^J^NOI^N^<714> ;Justify r/h margin T2 unless line < T1
N N ^N^^
N W '^NC^NS^N\^NM-^ND^N^<346>^UQ T$^JA^J^R^NR1^NOD^NR2^NOE^J^[D1 32000^J^N^<364>
n327 y ^P^@^Jy ^P^M ^P^J^Jy ^<200>^Jy ^<201>^Jy ^<202>^Jy ^<203>^Jy ^<204>^Jy ^<205>^Jy ^<206>^Jy ^<207>^Jy ^<210>^Jy ^<211>^Jy ^<212>^Jy ^<213>^Jy ^<214>^Jy ^<215>^Jy ^<216>^Jy ^<217>^Jy ^<220>^Jy ^<221>^Jy ^<222>^Jy ^<223>^Jy ^<224>^Jy ^<225>^Jy ^<226>^Jy ^<227>^Jy ^<230>^Jy ^<231>^Jy ^<232>^Jy ^<233>^Jy ^<234>^Jy ^<235>^Jy ^<236>^Jy ^<237>^Jy ^<240>^Jy ^<241>^Jy ^<242>^Jy ^<243>^Jy ^<244>^Jy ^<245>^Jy ^<246>^Jy ^<247>^Jy ^<250>^Jy ^<251>^Jy ^<252>^Jy ^<253>^Jy ^<254>^Jy ^<255>^Jy ^<256>^Jy ^<257>^Jy ^<260>^Jy ^<261>^Jy ^<262>^Jy ^<263>^Jy ^<264>^Jy ^<265>^Jy ^<266>^Jy ^<267>^Jy ^<270>^Jy ^<271>^Jy ^<272>^Jy ^<273>^Jy ^<274>^Jy ^<275>^Jy ^<276>^Jy ^<277>^Jy ^<300>^Jy ^<301>^Jy ^<302>^Jy ^<303>^Jy ^<304>^Jy ^<305>^Jy ^<306>^Jy ^<307>^Jy ^<310>^Jy ^<311>^Jy ^<312>^Jy ^<313>^Jy ^<314>^Jy ^<315>^Jy ^<316>^Jy ^<317>^Jy ^<320>^Jy ^<321>^Jy ^<322>^Jy ^<323>^Jy ^<324>^Jy ^<325>^Jy ^<326>^Jy ^<327>^Jy ^<330>^Jy ^<331>^Jy ^<332>^Jy ^<333>^Jy ^<334>^Jy ^<335>^Jy ^<336>^Jy ^<337>^Jy ^<340>^Jy ^<341>^Jy ^<342>^Jy ^<343>^Jy ^<344>^Jy ^<345>^Jy ^<346>^Jy ^<347>^Jy ^<350>^Jy ^<351>^Jy ^<352>^Jy ^<353>^Jy ^<354>^Jy ^<355>^Jy ^<356>^Jy ^<357>^Jy ^<360>^Jy ^<361>^Jy ^<362>^Jy ^<363>^Jy ^<364>^Jy ^<365>^Jy ^<366>^Jy ^<367>^Jy ^<370>^Jy ^<371>^Jy ^<372>^Jy ^<373>^Jy ^<374>^Jy ^<375>^Jy ^<376>^Jy ^<377>^Jy ^P^A^Jy ^P^B^Jy ^P^P^Jy ^P^D^Jy ^P^E^Jy ^P^F^Jy ^P^G^Jy ^P^H^Jy ^P^K^Jy ^P^L^Jy ^P^N^Jy ^P^O^Jy ^P^P^Jy ^P^Q^Jy ^P^R^Jy ^P^S^Jy ^P^T^Jy ^P^U^Jy ^P^V^Jy ^P^W^Jy ^P^X^Jy ^P^Y^Jy ^P^Z^Jy ^P^[^Jy ^P^^^Jy ^P^]^Jy ^P^\^Jy ^P^_^Jy ^P^?^J^NU
n367 ^NC^NS^N\ ^ND^N^<222>1 999999^J^ND^N^<210>^ND^N^A^NU
N227 ^NC^NS^N\^NM-^UN 4 ^*NU^J^ND^N^<255>^N^P^W
N707 ^NB2^NS^Y^@^NOA^NYA^J^NC^NU^NA^N^<706>^NG ^N^<702>^NG>^N^<700>^F^NA^NS^Y^@^NPC^N^<702>^N^<703>
/*
/* For editting reply mails (where the original lines start '> ') - if
/* we find one of these then just go to the next line
/*
N702 ^Z^Y^NP2^H^F^F^N^<704> ;New line:- first see if not too long
N703 ^[J-2,1,32000^J^NRA^E ^E^F^N^<704> ;We know 1st wd will fit
N704 ^NA^NS^Y^@^NP2^N^<705>^NA^N^<707>^F^F^N^<704> ;Work along line
* n705: This word doesn't fit. If it's the only word on the line, leave as_is.
*       Otherwise, split line at preceding word boundary.
N705 ^B^NP1^NL^J^N^<701>X^?^T^N^<702>
N706 ^NA^NS^N^<710>^J^NC^NU^N^<706> ;Accept extra blank lines until non blank
N710 ^NG>^N^<700>^N^<702> ;Leave lines starting "> " alone also
N711 ^NOJ^Z^NB2^NL^J^NUX^NRJ^NI^NU
N712 ^B^NPI^NS^N^<713> ^ND^N^<711>^N^<714>^N^<712>
N713 ^G.^NA^N^<712>^X^NG ^NS^N^<713>^ND^N^<711>^N^<714>^N^<713>
N714 M-^J^NC^NU^Z^NP1^NL^[^N^<714>X^NB2^NL^[^N^<714>X^E^H^N^<713>
N250 ^J^NC^NU^N^<251>
N251 ^NA^N^<250>^NG.^N^<250>^I^N^<250>
N252 L.,,,,1^J^NC^NU^D^ND^N^<253>^N^<252>^E^ND^N^<366>) ^J^ND^N^<266>^N^<252>
N253 ^NG ^NS^NL^NI^NU^NG.^NS^N^<254>^JN166  1^*NU^JM-1^J^D^N^<253>
N254 ^NG@^NS^NL ^N^<253>X^NG~^NS^NL^I ^N^<253>^NG^*^NL^J^NUX!^N^<253>
N255 ^NU ; Used by the ^N^P^W LQP suite
N364 T^ND^N^<340>,^ND^N^<341>^JA^J^R^NR1^NOA^NR2^NOB^N^<363>
N363 ^NYA^NRC^X^NO2^[E ^ND^N^<255>^J^ND^N^AM1 9999^J^ND^N^<251>G1^J^N^<362>
N362 ^ND^N^<252>G2^J^ND^N^<361>^N^<352>
N361 N165 ^ND^N^<342>^*NU^J^N^<360>
N360 ^ND^N^<265>^N^<357>M-^J^NC^NU^NG!^NS^NL^D^T^N^<356>^[^N^<360>
N357 M-^J^NC^NU^[^N^<361>
N356 ^J^ND^N^<265>^N^<361>M-1^J^T^N^<356>
N352 Y !,,,,,1^JT1 500^JA^J^R^NRD^NO1^NRE^NO2^[^N^<355>
N355 I1^J^<201>^<201>^J^[^ND^N^<354>G1^J^N^<334>
N334 L^_^J^NC^N^<330>^Z^T^A^H+^NG ^F^@^N^<333>
N333 ^NG^_^NS^NL ^N^<332>X ^NA^NS^NL^J^N^<334>X^N^<333>
N332 ^NA^NS^NL^J^N^<331>X^NG^_^NS^NL ^N^<333>X_^N^<332>
N331 M-^J^NC^N^<330>^X^NA^NS^NL^[^N^<334>X^Z^T^A^H+^NG ^F^@^N^<332>
N330 Y ^_ ' '^JS ^ND^N^<255>.DRAFT^J^NU
N354 M-^J^NC^NU^E1^T^JI-1^J^ND^N5^[M-^J^E ^JN165 ^ND^N^<342>^*NU^J^N^<353>
N 5 '^ND^N4^NU^N^<547> ;Header unless page 1 and ^N^W^W
N 4 '^NI^NU ; Gets modified if ^N^W^W
N353 ^ND^N^<265>^N^<351>M-^J^NC^N:^E ^J^N^<353>
N : '^ND^N4^NS^N^<335>N 4 ^*NI^*NU^J^NU
* N135 pads the last page to 54 or whatever lines then outputs footer
N335 A^J^J^[^ND^N^<265>^NS^N^<335>D-1^JA^J^ND^N^<350>^[^NU
N351 ^ND^N4^N9M-^J^NC^NS^NLA^@^NL^[I-1^J^ND^N^<350>^[^N^<354>
N 9 'N 4 ^*NI^*NU^J^N^<354> ; ^N^W^W foot of 1st page
N350 ^J^J^ND^N^<344>^ND^N^<267>^ND^N^<345>^J^[^N^<337> ; FOOTER start
N267 Page ^NU ; Put required footer pagination text here
N337 N143 ^ND^N^<366>^*NU^J^ND^N^<345>^NM^<366>^U^N^<336>
N336 ^ND^N^<266>^ND^N^<366>^NM^<345>^UN166 ^ND^N^<343>^*NU^J^NU
N344                                 ^NU
N343 ^NU ; Dump for ^N^<166>
N547 ^T^T^T^NU ; Modify to HEADER lines
N346 ^U1^NM^<345>^NU ; Put page count initialising here
N345 ^NU ; Reserved for page number
N340 4^NU ; Amend for different indenting (4 => indent 4)
N341 80^NU ; Amend for different page width (80 => 79 width)
N342 53^NU ; Amend for different page length (53 => 54 length)
Z
 The ^N^W^W macro is similar to the ^N^P^W macro described below,
except that headers and footers are not output on the first page, nor is
the page count incremented.
 The ^N^P^W macro produces a document for LQP spooling.
 On invoking the macro, the editor should be in command mode with the
<filename> the only thing on the command line. The document is formatted
to fit into 79 columns (or change N341 in the macro suite if this is not
suitable), all lines except those starting '.' then being indented to
N340 spaces. The lines starting '.' are treated specially, as follows:
 The characters following the '.' are treated as special editing
characters, terminated by ' ' or a character which is not special:-
     . - reset item counter to ' 1'
     ^ - take a new page. "Pages" are blocks of 54 lines : set by the
          definition of N142 in the macros
     ~ - Tab in
     @ - Single Space in
 space - insert item count, then increment item count.
 The effect of having no special characters after the '.' is that the
following line is not indented, and so stands out. This feature was
introduced for headings. If it is desired to indent a line less than the
normal default amount described above, use the '@' character.
 Underlining is activated by a ^_ and deactivated by another. The ^_
translates to a space in the spool file.
 Headers & Footers:- when N147 or N150 is invoked, the editor is in an I
or A command and header or footer lines respectively as defined in the
macro may be inserted. N146 is invoked at macro start so that page
numbering &c can be reset. As the counters N166 & N165 are used
elsewhere by the macro suite, these must be saved and restored should
the header / footer macros wish to use them. N146 should begin with a
^U.
The ^N^WW macro attempts to strip a WORD doc down to something readable when it
is editted in BINARY mode.
The ^N^Ww macro finishes the job when editting in TEXT mode
