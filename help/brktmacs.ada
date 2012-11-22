* $Id: brktmacs.ada,v 1.3 2012/10/16 23:49:35 dunc Exp $
fd y
N I '  ^NU ; indent 2 spaces
N H '^NC^N\^?^?^NU
N E '^N\   ; no bracketing defined yet
N L '^N\   ; no fancy else
N Y '^NE^H^NA^NS^N^<422>-- ^NU ; Ada comment at start of line
N422 ^NOb^Z^NB2^NS^T^I^I^NB2^I^@--  ^NU
N N '^N^^
N040 !umake 2>&1|tee hee|grep --line-buffered error|egrep --line-buffered -v 'No errors|cdc_fatal_error_handler|rec_remvol_error_states|scm_[bo]sc_error|javadoc'^J^NU
x
t 8
x
fm -l /* ADA doesn't like tabs
fm +s /* Minimise diffs
z
