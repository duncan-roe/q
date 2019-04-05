/* B A CK T I C K . H */
#ifndef BACKTICK_H
#define BACKTICK_H

/* Items for the "backtick" active pseudos 04014 & 04015 */

/* Headers required by this header */

#include "typedefs.h"

/* Macro definitions */

#define STDOUT_MACRO_IDX LAST_PSEUDO
#define STDERR_MACRO_IDX LAST_PSEUDO - 1

/* Global variables */

extern uint8_t stdoutbuf[Q_BUFSIZ];
extern uint8_t stderrbuf[Q_BUFSIZ];
#endif
