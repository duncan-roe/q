fd y /* H SCMACS.INFO for extra info on these...
fm-t +s -* +#
t7 36 81
o n
N A '^L^I^?^NE^E+^E^NU FORTRAN continuation
N B '^NC^NS^N\A^J^[V1^J^NU "BOTTOM"
N C '''^ND^N^<4002>''^NU ; Access current edit file name. Call by ^N^C^C / ^N^P^P
N D '^J^NC^NL^ND^N"M-2^J^N^G Join previous line
N E '^F^F^F^X^Q
N F '^Z^NOA^A^NRA^F^K^NU ^F with recall
N G '^Z ^NO9^J^NC^NL^ND^N"J-1^J^NR9^NU ;Join next line ;^NH sets columns
N I '^H^O^B^T^I^I^N^J ;Split "/*" comment to next line (OBS)
N J '^ND^N^O^J^NU ;Single "/*" comment beautify - call by ^N^M
N K '^D^NA^NU^NG ^NS^N^K^D^NU Word delete
N N '^N^^ ; Can change for convenience
N O '^NOa^H^O^S^X^ND^N^U^NRa^NU Comment beautify - no n/l
N P ^N^C^C
N Q '^NC^NS^N\g-^ND^N^<4004>^Jv^J^NU View previous screenful
N R '^H^NA^N^<1201>^NM^<1201>^NU ;Remember/recall command or line
N S '^NOD^G ^NOC^Z^NOB^NRC^K^NRD^S^R^NRB^K^NRC^NU ;Word to u/c
N T '^NC^NS^N\^UG 1^JV 0^J^NU "TOP"
N U '^Q^NO9^ND^N^<527>^NR9^NU l/c except sentence strt
N527 ^ND^N'^NU^F^NA^NU^S^X^Q^NG.^F^@^N^<527>
N V '^NC^NS^N\g+^ND^N^<4001>^Jv^J^NU View next screenful
N X '^Z^NOA^A^NRA^X^K^NU ^X with recall
N Y '^NE^I^NP2^NS^NL^T^I^I^@^E/* ^E^NU FORTRAN comment
N Z '^Z^B^S^J
N ^ '^NC^NL^J^[^NS^NL^ND^N^^^[M-2,32000^J^NU ;Back up 1 line when modifying
N _ ^P^M^NU
N ! '^NA^NU^NG ^NS=^Y^X^N!
N " '^[G-1^J^NU
N47 '^G.^NA^NU^X^NG ^NL^F^N''^@^NI^NU
N $ 'curently spare^N\
N & '^NC^NS^N\!rcsdiff ^ND^N^<522> "^ND^N^<4002>"|k -E^J^NU
N246 ^NC^NS^N\!cvs diff ^ND^N^<522> "^ND^N^<4002>"|k -E^J^NU ;NW&
N * '!/bin/ls -l "^ND^N^<4002>"^J^NU
N ( '^NC^NS^N\^NFa!co^ND^N^<1301> -l "^ND^N^<4002>"^Jq '^ND^N^<4002>'^J^NNa^NU
N ) '^NC^NS^N\^NFas^J!ci^ND^N^<1302> -u "^ND^N^<4002>"^Jq '^ND^N^<4002>'^J^NNa^NU
N @ '^NO9^ND^N!^NR9^NU Non-space -> equals
N 0 '^NC^NS^N\T,7,36^J^N^<1271> FORTRAN Comment cc36 + beautify
N 6 '^NC^N^<1277>^N\ FORTRAN C-comment beautify
N 7 '^NC^NS^N\^H^NA*^@^NM^<1201>^U^<ZAM>^<RST>^ND^N^<4005>^NM^<7000>^U^<PS8>^<PSH 0>^<SUB>^<POP 0>^ND^N^<7000>^NM^<1073>^U^ND^N^<777>^ND^N^<1201>^J
N 8 '^NC^NS^N\^H^NA*^@^NM^<1201>^U^ND^N^<1405>^NM^<1073>^U^ND^N^<777>^ND^N^<1201>^J
N 9 'fi fl ^ND^N2^*J^*NC^*NU^*[fb^*Jg-1^*Jv^*Jv0^*Jg+1^*Jfv^*J^*NU^J^NU
N 2 '!!Put token to match here!!^N\ ; e.g. n2 ioctl^NU or ioctl^NM2
N ; '^NC^NS^N\^H^NE^NM ^N^<514>
N < '^NC^NS^N\g-1^Jv^J^NU
N > '^NC^NS^N\g+1^Jv^J^NU
N201 ^A^J^NC^NU^[^N^<201> ;Repeat last LOCATE till no more found
* n206 - (^N^W^F) search current file for all occurrences of command line.
*        grep options in n1404
N206 ^NC^NS^N\^NM^<1202>^U!grep ^ND^N^<1404> -- '^ND^N^<1202>' "^ND^N^<4002>"^J^NU
N207 ^NC^NS^N\^ND^N^<1001>fm-dw^Js^J!runindent.sh "^ND^N^<4002>"^Jq '^ND^N^<4002>'^J^ND^N^<1011>^ND^N1s^J^NU
N210 ^E^ND^N^<1201>^J^NC^NU^N^<210> ;Insert ^*N^<1201> at Home posn
N222 ^NC^NS^N\^NM^<1201>^UM ^NU ;Define string to be appended/inserted
N224 ^NC^NS^N\!ci -l "^ND^N^<4002>"</dev/null^Js^J!ci -l "^ND^N^<4002>"<~/spaces^JQ^J^N^<224> ;Check in all args
N225 ^NC^NS^N\!ci -d -t- "^ND^N^<4002>"^J!rcs -U "^ND^N^<4002>"^J!co -M "^ND^N^<4002>"^J^NU ;Make known to RCS - non-strict locking
N230 ^NC^NS^N\^ND^N^<4002>^[!echo "^P^[]0;"^ND^N^<4002>"^P^G"^J^NU ; Set xterm title to file (stuff at front guards against sending bad Ec seq when no file)
N232 ^Z^ND^N^<1201>^J^NC^NU^N^<232> ;Append ^*N^<1201> to lines
N241 ^NC^NS^N\^NM^<1201>^U^ND^N^<506>^NU ; NW! delete lines found by L
N255 ^NC^N\^H^ND^N^<515>^NU
N265 ^NC^NS^N\^NE^H^NA^NS^N\^NOI^ND^N^<365>^Y^N^<511> ;Decr. SPZ ^<365>
N266 ^NC^NS^N\^NE^H^NA^NS^N\^NOI^ND^N^<366>^Y^N^<507> ;Decimal inc ^<366>
* n347 (^N^Wg) Check in current source with message 1103
N347 ^NC^NS^N\!ci -u -m'^ND^N^<1103>' "^ND^N^<4002>"^J^NU
* n362 - (^N^Wr) Multi-file replacer. If N1100 returns at once, got to next file.
*        Else invoke n1101.
*        Go to line 1 & turn off indenting before calling each macro
* n360 & n361 - generate n1100 & n1101 for n362
N360 ^NC^NS^N\^NM^<1202>^Un1100 ^ND^N^<1401>l '^ND^N^<1202>'^*J^*NC^*NU^*[^*NI^*NU^J^NU
N361 ^NC^NS^N\^NM^<1201>^Un1101 ^*ND^*N^*<1106>^ND^N^<1401>y '^ND^N^<1202>' '^ND^N^<1201>'^*J^*NU^J^NU
N362 ^NC^NS^N\^[n523 ^ND^N^<4000>^*NU^J^ND^N^<524>^[fm ^ND^N^<523>^J^NU
* n367 (^N^Ww) Check in current source with message 1102
N367 ^NC^NS^N\!ci -u -m'^ND^N^<1102>' "^ND^N^<4002>"^J^NU
N523 !!! SCRATCH - DEFINED BY OTHER MACROS !!!
N524 o n^Jg1^J^ND^N^<1100>^N^<525>g1^J^ND^N^<1101>^ND^N^<1402>^N^<525>
N525 ^[q^J^N^<524>
N1100 !! put DECIDER macro here !!
N1101 !! put EDITOR macro here !!
N1102 Warning-free with -Wall -Wmissing-prototypes -Wstrict-prototypes^NU
N1103 After runindent^NU
N1104 !co -l "^ND^N^<4002>"^Jq $0^J^NU
N1105 !ci -u -m"^ND^N^<1202>+>^ND^N^<1201>" "^ND^N^<4002>"^J^NU
N1106 ^NU ; Can change to enable RCS / CVS
N1107 ^NU ; Can change to enable RCS / CVS
* n322 (^N^WR) Enable "R"CS in ^N^Wr
N322 n1106 ^*ND^*N^*<1104>^*NU^Jn1107 ^*ND^*N^*<1105>^*NU^J^NU ; ^N^WR global replace does co/ci
* n325 (^N^WU) "U"ndo RCS / CVS enable in ^N^Wr
N325 n1106 ^*NU^Jn1107 ^*NU^J^NU
* n363 - (^N^Ws) show what files $C expands to
N363 ^NC^NS^N\! for i in ^ND^N^<1403>;do echo $i;done|pr -n -t^J^NU
N1070 /* 107X - DEFAULT VALUES FOR VARIOUS MACROS
N1073 80^NU ; Ruler width (set by macros)
* n372 (^N^Wz) - set up N N to globally uppercase its line contents
N372 ^NC^NS^N\n016 ^*NM^*<526>^*Uq $1^*J^*ND^*N^*<526>^*ND^*N^*<360>^*ND^*N^*<526>^*H^*S^*ND^*N^*<361>^*ND^*N^*<362>^*NU^J^NU
N526 !!! SCRATCH - DEFINED BY OTHER MACROS !!!
N507 ^NG9^NS^N^<503>^ND^N^<510>^N^<507>^NU
N510 0^Y^NPI^NS^NL^Y^NU^@^NI^E1^NM^<366>^U^NU
N511 ^NG0^N^<512>^NG10^Y^NG21^Y^NG32^Y^NG43^Y^NG54^Y^NG65^Y^N^<513>
N512 ^NPI^NL^U^NU^@9^Y^Y^N^<511>
N513 ^NG ^N^<512>^NG76^Y^NG87^Y^NG98^Y^NM^<365>^U^NI^NU
N300 ^NC^NS^N\^NE^H^NA^NS^N\^NOI^ND^N^<366>^Y^N^<501> ;Inc HEX ^<366>
N501 ^NGF^NS^N^<502>^ND^N^<510>^N^<501>^NU
N502 ^NGEF^Y^NGDE^Y^NGCD^Y^NGBC^Y^NGAB^Y^NG9A^Y^N^<503>
N503 ^NG89^Y^NG78^Y^NG67^Y^NG56^Y^NG45^Y^NG34^Y^NG23^Y^NG12^Y^NG01^Y^NG 1^Y^NM^<366>^U^NU
N304 ^NC^NS^N\^NE^H^NA^NS^N\^NOI^ND^N^<366>^Y^N^<505> ;Inc OCT ^<366>
N505 ^NG7^NS^N^<503>^ND^N^<510>^N^<505>^NU
N506 ^ND^N^<1201>^J^NC^NU^[D-1^J^N^<506>
N365 0^NU ;Put required iteration count here & use <265>
N366 1^NU ;Put initial value&format here & use <266>, <300>, <304>
N514 ^NC^NS^[^@^G;^NA^NL^D^T^N^<514>^J^NU
N515 ^G^H^NA^NU^D^D^D^?^NG^H^Y^@^N^<515>
N522 ^NU ; RCSDIFF options for Ctl-N &
* N777 - Draw ruler of length ^N^<1073>. Sets up initial values then calls N776
*        To fit the screen exactly, N1073 should be 8 less than screen width (as returned by N4005)
*   Memory locations:-
*   N7000 - ruler width
*   N7001 - constant 10
*   N7002 - constant 100
*   N7003 - constant 1000
*   N7004 - How many spaces to put before next 10's marker
*   N7005 - How many 10's markers left to do or Value of next single digit
*   N7006 - Value of next 10's marker
* N776 If more markers to do, increment marker value.
*      Work out how many spaces before next marker: there are only 8 after the initial '*'
*      - there are 9 if the marker value is less than 10
*      - otherwise there are 8 if the marker value is less than 100
*      - otherwise there are 7 if the marker value is less than 1000
*      - otherwise there are 6.
*      Call N775 to o/p these spaces.
*      Output the marker.
*      Repeat.
* N775 - Outputs spaces
* N774 - Output digits 0 - 10 repeatedly until all done
N777 ^<ZAM>^<RST>^ND^N^<1073>^NM^<7000>^U10^NM^<7001>^U100^NM^<7002>^U1000^NM^<7003>^U^<PSH 1>^<PSH 0>^<DIV>^<POP 5>*    ^ND^N^<776>^<POPN>^J*    ^<PSH 0>^<POPX>^ND^N^<774>^J^NU
N776 ^<PSH 5>^<S1>^<SGE>^NU^<POP 5>^<PSH 1>^<POPX>^<S1X>^<PSH 6>^<A1>^<POP 6>^<PSH 6>^<PSH 1>^<SUB>^<SGT>^<S1X>^<POPN>^<PSH 6>^<PSH 2>^<SUB>^<SGT>^<S1X>^<POPN>^<PSH 6>^<PSH 3>^<SUB>^<SGT>^<S1X>^<POPN>^ND^N^<775>^ND^N^<7006>^N^<776>
N775 ^<S1X>^<SXGE>^NU ^N^<775>
N774 ^<S1X>^<SXGE>^NU^<PSH 1>^<PSH 5>^<A1>^<POP 5>^<PSH 5>^<SUB>^<SLT>^NS^NL^<PS0>^<POP 5>^<POPN>^ND^N^<7005>^N^<774>
N1001 ^NC^NS^N\^[^NFa^ND^N^<4000>^NM^<1021>^[^NU ; Env save #1
N1011 ^NC^NS^N\g ta^Jfm^ND^N^<1021>^J^NU ; Env restore #1
N1021 !!SCRATCH - USED BY ENV DUMP #1
N1200 /* 12XX - USED AS SCRATCH SPACE AND FOR OTHER FORMER KEYBOARD INTERNAL MACROS
N1201 /* 1201 - REPLACES ^N<
N1202 /* 1202 - REPLACES ^N>
N1271 ^ND^N^<1272>^NB2^I^@^NB2^I^@^NP2^NS^NL^ND^N^<1273>^ND^N^J^N^<1271>
N1272 L ''/*''^J^NC^NX^NU
N1273 ^Y^NG ^NS^N^<1274>^Y^NG ^N^<1275>^X^N^<1274>
N1274 ^X^T^I^I^NU
N1275 ^D^X^NP2^N^<1273>^NU
N1277 L C,,,,1^J^NC^NX^F^NA^NL^S^X^Q^@^J^N^<1277>
N1300 /* 13XX - USED FOR MISCELLANEOUS FLGS. Change to ^NU for no effect
N1301  -M^NU  ;for ^N(
N1302  -d -M^NU  ;for ^N)
N1400 /* 14XX - MACRO USER-SETTABLE DEFAULTS (I.E. PARAMETERS)
N1401 f^NU ; Define to be ^NU for l & y instead of fl & fy (^N^Wp & ^N^Wq)
N1402 s^J^ND^N^<1107>^NU ; Override with "b", &c. as required
N1403 $C^NU ; override with invocation wildcard (^N^Ws)
N1404 -wn^NU ; grep options for ^N^W^F
N1405 80^NU ; Ruler width (^N8)
N1500 /* 15XX - RESERVED FOR MACRO PROGRAMS (*.qm)
