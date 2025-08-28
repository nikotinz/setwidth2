#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

/* .C routines */
void setwidth2_Start(int *verbose);
void setwidth2_Stop(void);

/* .Call routines */
SEXP setwidth2_tick(void);

static const R_CMethodDef CEntries[] = {
  {"setwidth2_Start", (DL_FUNC) &setwidth2_Start, 1},
  {"setwidth2_Stop",  (DL_FUNC) &setwidth2_Stop,  0},
  {NULL, NULL, 0}
};

static const R_CallMethodDef CallEntries[] = {
  {"setwidth2_tick",  (DL_FUNC) &setwidth2_tick,  0},
  {NULL, NULL, 0}
};

void R_init_setwidth2(DllInfo *dll)
{
  R_registerRoutines(dll, CEntries, CallEntries, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
}
