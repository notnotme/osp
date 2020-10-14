#---------------------------------------------------------------------------------
# GIT
#---------------------------------------------------------------------------------
GIT_VERSION := $(shell git describe --abbrev=0 --tags)
GIT_COMMIT  := $(shell git rev-parse --short=8 --verify HEAD)
BUILD_DATE  := $(shell date --iso-8601)
ifeq ($(strip $(GIT_VERSION)),)
	GIT_VERSION := "Unknown"
endif
ifeq ($(strip $(GIT_COMMIT)),)
	GIT_COMMIT := "No commmit found"
endif
ifneq ($(strip $(shell git status --porcelain 2>/dev/null)),)
	GIT_VERSION := $(GIT_VERSION)-dirty
endif
