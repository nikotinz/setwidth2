
# This file is part of setwidth2 R package
# 
# It is distributed under the GNU General Public License.
# This file is part of setwidth R package
# 
# It is distributed under the GNU General Public License.
# See the file ../LICENSE for details.
# 
# (c) 2011-2013 Jakson Aquino: jalvesaq@gmail.com
# 
###############################################################

.onLoad <- function(libname, pkgname) {
  if (is.null(getOption("setwidth2.verbose")))
    options(setwidth2.verbose = 0L)

  # Start native side (initial width + Unix eventloop if available)
  if (is.loaded("setwidth2_Start", PACKAGE = "setwidth2")) {
    .C("setwidth2_Start", as.integer(getOption("setwidth2.verbose")), PACKAGE = "setwidth2")
  }

  # Decide whether to run an R-side timer/task (Windows or Unix fallback)
  need_timer <- (.Platform$OS.type == "windows") || isTRUE(getOption("setwidth2.force_timer"))

  if (need_timer && is.loaded("setwidth2_tick", PACKAGE = "setwidth2")) {
    # Prefer 'later' so it ticks even while idle
    if (requireNamespace("later", quietly = TRUE)) {
      ns <- asNamespace("setwidth2")
      ns$.setwidth2_timer_active <- TRUE
      ns$.setwidth2_interval_sec <- 0.2

      tick <- function() {
        if (isTRUE(get(".setwidth2_timer_active", envir = ns))) {
          .Call("setwidth2_tick", PACKAGE = "setwidth2")
          later::later(tick, get(".setwidth2_interval_sec", envir = ns))
        }
      }
      later::later(tick, 0)
      options(setwidth2.task.id = NULL)  # no task callback when using later
    } else {
      # Fallback: refresh after each top-level command
  id <- addTaskCallback(function(...) { .Call("setwidth2_tick", PACKAGE = "setwidth2"); TRUE },
            name = "setwidth2-task")
  options(setwidth2.task.id = id)
    }
  }
}

.onUnload <- function(libpath) {
  # Stop R-side timer/task
  ns_name <- "setwidth2"
  if (ns_name %in% loadedNamespaces()) {
    ns <- asNamespace(ns_name)
    if (exists(".setwidth2_timer_active", envir = ns, inherits = FALSE)) {
      assign(".setwidth2_timer_active", FALSE, envir = ns)
    }
  }
  id <- getOption("setwidth2.task.id", NULL)
  if (!is.null(id)) {
    removeTaskCallback(id)
    options(setwidth2.task.id = NULL)
  }

  # Stop native side
  if (is.loaded("setwidth2_Stop", PACKAGE = "setwidth2")) {
    .C("setwidth2_Stop", PACKAGE = "setwidth2")
  }

  library.dynam.unload("setwidth2", libpath)
}
