# setwidth2 R package <img src="setwidth2.png" alt="setwidth2 logo" width="120"/>

*Inspired by and based on the original [setwidth](https://cran.r-project.org/web/packages/setwidth/index.html) package by Jakson Alves de Aquino, but extended for cross-platform support (Linux, macOS, Windows).*

Automatically set the value of `options("width")` when the terminal emulator is resized. This package is inspired by and based on the original 'setwidth' package, but extends its functionality to be fully cross-platform (Linux, macOS, Windows) for interactive R sessions.

## Description

`setwidth2` automatically updates the R output width to match the current terminal size. It is inspired by and based on the original 'setwidth' package, but extends its functionality to be fully cross-platform. It is designed for use in interactive terminal sessions and should not be used with graphical R interfaces (such as RGui, RStudio, RKward, JGR, Rcmdr, etc.) that manage their own output display. The package installs a platform-specific event handler in C that updates `options("width")` whenever the terminal size changes. On Unix-like systems, this is done by polling the terminal size using the event loop (if available) or by using a timer fallback. On Windows, a timer is used to periodically check the console width. The handler is only activated if R is running interactively and the terminal environment is suitable.

## Installation

### From GitHub (development version)
```r
# install.packages("remotes")
remotes::install_github("nikotinz/setwidth2")
```

## Usage

The package works automatically when loaded:

```r
library(setwidth2)
```

You can control verbosity with:

```r
options(setwidth2.verbose = 1) # Print startup message
options(setwidth2.verbose = 2) # Print error message when unable to set width
options(setwidth2.verbose = 3) # Print width value
```

To disable automatic width setting:

```r
detach("package:setwidth2", unload = TRUE)
```

## Authors
- Jakson Alves de Aquino (author of the original setwidth package)
- Dominique-Laurent Couturier
- Nikita Mozgunov (cross-platform extension)

## License
GPL (>= 2)
