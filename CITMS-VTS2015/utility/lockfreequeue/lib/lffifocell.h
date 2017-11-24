#ifndef __lffifocell__
#define __lffifocell__

typedef struct fifocell {
    struct fifocell* volatile link;	/* next cell in the list */
    long value[3];					/* any data here */
} fifocell;

#endif
