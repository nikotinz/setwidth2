##' @title setwidth2 package
##' @description See the package documentation: \code{?setwidth2-package}
##' @keywords internal
##' @name setwidth2
NULL
#' @title Automatically set the value of options("width") when the terminal emulator is resized
#' @description
#' This package should not be used with Graphical User Interfaces, such as Windows RGui, RStudio, RKward, JGR, Rcmdr and other interfaces which have their own engine to display R output. The functions of this package only work if R is compiled for Linux, macOS, or Windows systems and it is running interactively in a terminal emulator. The terminal emulator might have been called by a text editor, such as Vim, Gedit, Kate or Geany.
#'
#' @details
#' The package will print information on the R Console if its `setwidth2.verbose` option was set to a numeric value bigger than zero:
#'
#' \preformatted{
#' options(setwidth2.verbose = 1) # Print startup message
#' options(setwidth2.verbose = 2) # Print error message when unable to set width
#' options(setwidth2.verbose = 3) # Print width value
#' }
#'
#' The package does not have any user visible R function. When it is loaded, the SIGWINCH signal starts to be handled by a C function that updates the value of `options("width")`. The handler will not be activated if `interactive() == FALSE` or the value of the environment variable `TERM` is either empty or `"dumb"`.
#'
#' To manually test whether the package is working properly on your system you may repeatedly resize the terminal emulator and print a long vector, like 1:300.
#'
#' To disable the automatic setting of `options("width")` do:
#'
#' \preformatted{
#' detach("package:setwidth2", unload = TRUE)
#' }
#'
#' @author Jakson Alves de Aquino (jalvesaq@gmail.com), with some code copied from Vim.
#' @seealso \code{colorout} package colorizes R output when running in a terminal emulator.
#' @examples
#' options(setwidth2.verbose = 1)
#' print(getOption("width"))
NULL
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

  # Load native library
  library.dynam("setwidth2", pkgname, libname, local = FALSE)

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
