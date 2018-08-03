R_DYNTRACE_HOME := ../R-dyntrace
R_DYNTRACE := $(R_DYNTRACE_HOME)/bin/R
R_CMD_CHECK_OUTPUT_DIRPATH := /tmp

export R_ENABLE_JIT=3
export R_COMPILE_PKGS=1
export R_DISABLE_BYTECODE=0
export R_KEEP_PKG_SOURCE=1

all: install

build: clean document
	$(R_DYNTRACE) CMD build .

install: build
	$(R_DYNTRACE) CMD INSTALL --preclean --with-keep.source promisedyntracer*.tar.gz

clean:
	rm -rf promisedyntracer*.tar.gz
	rm -rf *.Rcheck
	rm -rf src/*.so
	rm -rf src/*.o

document:
	$(R_DYNTRACE) -e "devtools::document()"

check: build
	$(R_DYNTRACE) CMD check --output=$(R_CMD_CHECK_OUTPUT_DIRPATH) promisedyntracer_*.tar.gz

test:
	$(R_DYNTRACE) -e "devtools::test()"


install-dependencies:
	$(R_DYNTRACE) -e "install.packages(c('withr', 'testthat', 'devtools', 'roxygen2'), repos='http://cran.us.r-project.org')"

.PHONY: all build install clean document check test install-dependencies
