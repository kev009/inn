/*  $Revision$
**
*/
#include <stdio.h>
#include <sys/types.h>
#include "configdata.h"
#include "clibrary.h"
#ifdef HAVE_WAIT_H
# include <wait.h>
#else
# include <sys/wait.h>
#endif


#if	defined(HAVE_UNION_WAIT)
typedef union wait	WAITER;
#if	defined(WEXITSTATUS)
#define WAITVAL(x)	(WEXITSTATUS(x))
#else
#define WAITVAL(x)	((x).w_retcode)
#endif	/* defined(WEXITSTATUS) */
#else
typedef int		WAITER;
#define WAITVAL(x)	(((x) >> 8) & 0xFF)
#endif	/* defined(HAVE_UNION_WAIT) */

PID_T waitnb(int *statusp)
{
    WAITER	w;
    PID_T	pid;

#if defined(HAVE_WAITPID)
    pid = waitpid(-1, &w, WNOHANG);
#else
    pid = wait3(&w, WNOHANG, (struct rusage *)NULL);
#endif	

    if (pid > 0)
	*statusp = WAITVAL(w);
    return pid;
}
