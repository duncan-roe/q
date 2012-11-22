* $Id: scmacs.pl,v 1.5 2012/10/11 10:01:49 dunc Exp $
fd y /* Perl macros that differ from default FTN ones
t 2 36
o y
fc n
ft_
fm-tw
N A '^Z^T  ^NU ; Continuation
N E '^NC^N\^NM^<1201>^Z^T{^T}^J^[i-^J^ND^N^<1201>^H^L^NM^<1201>^[m-1^J^Z^I# ^ND^N^<1201>^J^[i-1^J  ^NU Perl new bracket level
N I '^NC^N\^Z^T^L# ^NU ;Continue comment nxt lin
N L '^?^?^NU ;Perl "Erase" 1 indent level
N N '^N^^
N O '^NC^N\^NOa^H^ND^N^<407>^S^X^ND^N^U^NRa^NU ; Perl comment beautify - no n/l
N Y '^NE^H^NA^NS^N^<405>^L# ^NU Perl comment
N405 ^Z^X^NB2^I^@# ^NU
* N 0   "beautify" Perl source. Currently consists solely of pulling back
*       comments to cc1. File position is restored afterwards. ^N0
*       sets up tab b to the current file pos'n and tab c to be cc2...
N 0 '^NC^NS^N\^ND^N^<4000>^NM^<403>^Ufb^J^NFbo y^Ji1^JX^NOc^[^ND^N^<404>g tb^Jfm ^ND^N^<403>^J^NU
N403 !! SCRATCH - used by ^N0,
* N404: Main loop on lines. If we have a comment and it doesn't start in cc1
*       then pull it back.
N404 ^[m-^J^NC^NU^NG#^NS^N^<404>^NBc^N^<404>^L^J^N^<404>
* N405: Do a ^O equivalent for Perl. If line starts '#', look for 1st non-space
*       following; else look for a # then look for non-space
N407 ^NOx^H^NG#^N^<406>^NRx^G#^NA^NU^X^NA^NU^N^<406>
N406 ^X^NA^NU^NG ^N^<406>^NU
* N205 ^NC^N\^H^NA^NS^N\^[m-^J^NG}^NS^N\^X^K^Jm-2^J  ^NU ; strip comment from trlg "}"
N205 ^NC^N\^NFa^J^[g ta^Jm-1^J^NG}^NS^N\^X^K^Jm-2^J^NU ; strip comment from trlg "}"
Z
