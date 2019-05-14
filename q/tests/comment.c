/* This file tests ^N1 in c.qm.
    * This line should get pulled back
 * The next line should not get modified */
     *i = *j;                      /* Not part of a comment */
/* This is a comment */
    *i = *j;                       /* This is not */
