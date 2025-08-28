// src/setwidth.c
#include <R.h>
#include <Rinternals.h>

#ifndef _WIN32
  #include <R_ext/eventloop.h>   /* for types; symbols resolved via dlsym */
  #include <unistd.h>
  #include <sys/ioctl.h>
  #include <dlfcn.h>
#else
  #include <windows.h>
#endif

/* ---- module state ---- */
static int   setwidth2_initialized = 0;
static int   setwidth2_verbose     = 0;
static long  setwidth2_oldcolwd    = 0;

#ifndef _WIN32
/* eventloop symbols resolved at runtime (NULL if not available) */
static void *setwidth_poll_handle = NULL;
static void *(*p_addPolledEventHandler)(Rboolean (*)(void *), void *) = NULL;
static void  (*p_removePolledEventHandler)(void *) = NULL;
/* R_wait_usec is a global int inside libR; resolve its address */
static int   *p_R_wait_usec = NULL;
#endif

/* cache symbols for options(width=...) */
static SEXP sym_options = NULL;
static SEXP sym_width   = NULL;

/* poll cadence (microseconds) */
#ifndef SETWIDTH_POLL_USEC
#define SETWIDTH_POLL_USEC 150000  /* ~0.15s */
#endif

/* forward decl */
static void setwidth2_do(void);

/* ----- helpers ----- */
#ifndef _WIN32
/* set an R option from C: options(name = flag) */
static void set_option_flag(const char *name, int flag) {
    SEXP nm  = PROTECT(Rf_install(name));
    SEXP val = PROTECT(Rf_ScalarLogical(flag));

    /* Build call: options(name = val) */
    SEXP arg = PROTECT(Rf_lcons(val, R_NilValue));
    SET_TAG(arg, nm);

    SEXP call = PROTECT(Rf_lcons(Rf_install("options"), arg));
    Rf_eval(call, R_BaseEnv);

    UNPROTECT(4);
}
#endif

/* ----- platform width detection ----- */
static void get_console_size(int *rows, int *cols) {
    *rows = 0; *cols = 0;

#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
            *cols = (int)(csbi.srWindow.Right  - csbi.srWindow.Left  + 1);
            *rows = (int)(csbi.srWindow.Bottom - csbi.srWindow.Top   + 1);
        }
    }
#else
  #ifdef TIOCGWINSZ
    if (isatty(STDOUT_FILENO)) {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
            *cols = (int)ws.ws_col;
            *rows = (int)ws.ws_row;
        }
    }
  #elif defined(TIOCGSIZE)
    if (isatty(STDOUT_FILENO)) {
        struct ttysize ts;
        if (ioctl(STDOUT_FILENO, TIOCGSIZE, &ts) == 0) {
            *cols = (int)ts.ts_cols;
            *rows = (int)ts.ts_rows;
        }
    }
  #endif
#endif
}

/* ----- core action: apply options(width=cols) if changed ----- */
static void setwidth2_do(void) {
    int rows = 0, cols = 0;
    get_console_size(&rows, &cols);

    if (cols <= 0) {
        if (setwidth2_verbose > 1)
            REprintf("setwidth2: unable to detect terminal width\n");
        return;
    }
    if ((long)cols == setwidth2_oldcolwd) return;

    setwidth2_oldcolwd = (long)cols;

    if (!sym_options) sym_options = Rf_install("options");
    if (!sym_width)   sym_width   = Rf_install("width");

    SEXP val = PROTECT(Rf_ScalarInteger(cols));
    SEXP call = PROTECT(Rf_lang2(sym_options, val));
    SET_TAG(CDR(call), sym_width);
    Rf_eval(call, R_BaseEnv);
    UNPROTECT(2);

    if (setwidth2_verbose > 2)
        Rprintf("setwidth2: %ld columns\n", (long)cols);
}

/* Exposed tick for timers/task callbacks (defined on ALL platforms) */
SEXP setwidth2_tick(void) {
    R_ToplevelExec((void (*)(void *))setwidth2_do, NULL);
    return R_NilValue;
}


#ifndef _WIN32
/* Unix: polled callback used if eventloop is available */
static Rboolean setwidth2_poll_cb(void *data) {
    R_ToplevelExec((void (*)(void *))setwidth2_do, NULL);
    return TRUE;
}
#endif

/* ----- lifecycle ----- */
void setwidth2_Start(int *verbose)
{
    setwidth2_verbose = verbose ? *verbose : 0;
    if (setwidth2_initialized) return;

    /* initial application */
    setwidth2_do();

#ifndef _WIN32
    /* Try to resolve eventloop API at runtime */
    void *self = dlopen(NULL, RTLD_LAZY);
    if (self) {
        p_addPolledEventHandler    =
            (void *(*)(Rboolean (*)(void *), void *)) dlsym(self, "addPolledEventHandler");
        p_removePolledEventHandler =
            (void  (*)(void *))                        dlsym(self, "removePolledEventHandler");
        p_R_wait_usec =
            (int   *)                                  dlsym(self, "R_wait_usec");
        /* Keep 'self' open for symbol lifetime (do not dlclose) */
    }

    if (p_addPolledEventHandler && p_removePolledEventHandler && p_R_wait_usec) {
        *p_R_wait_usec = SETWIDTH_POLL_USEC;  /* variable, not a function */
        setwidth_poll_handle = p_addPolledEventHandler(setwidth2_poll_cb, NULL);
        if (!setwidth_poll_handle) {
            REprintf("setwidth2: failed to install polled handler\n");
        } else if (setwidth2_verbose) {
            REprintf("setwidth2 loaded (polling %d µs via eventloop)\n", (int)SETWIDTH_POLL_USEC);
        }
    } else {
        /* Eventloop not available; ask R side to start a timer/task */
        if (setwidth2_verbose)
            REprintf("setwidth2: eventloop unavailable; enabling R timer fallback\n");
        set_option_flag("setwidth2.force_timer", 1);
    }
#else
    if (setwidth2_verbose)
        REprintf("setwidth2 loaded (Windows timer/task mode)\n");
#endif

    setwidth2_initialized = 1;
}

void setwidth2_Stop(void)
{
    if (!setwidth2_initialized) return;

#ifndef _WIN32
    if (p_removePolledEventHandler && setwidth2_poll_handle) {
        p_removePolledEventHandler(setwidth2_poll_handle);
        setwidth2_poll_handle = NULL;
    }
#endif
    setwidth2_initialized = 0;
}
