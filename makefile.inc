# Registry where to push artifact image
DOCKER_TARGET_REGISTRY ?= harbor.transmit.im/oak/

# Registry for pulling builder and dependent images
DOCKER_SOURCE_REGISTRY ?= harbor.transmit.im/dockers/

# Image used in build-in-docker function
DOCKER_RUNNER_IMAGE_NAME ?= ubi

# Full build-in-docker runner image path including registry
DOCKER_RUNNER_IMAGE ?= $(DOCKER_SOURCE_REGISTRY)$(DOCKER_RUNNER_IMAGE_NAME)

# Artifact name and version generator. Default values are used only for local builds and are re-set on CI.
# Guess first part of artifact name based on git project name
GIT_PROJECT ?= $(shell GIT_PROJECT=$$(git remote get-url origin); GIT_PROJECT=$${GIT_PROJECT%/*}; GIT_PROJECT=$${GIT_PROJECT\#\#*/};echo $$GIT_PROJECT)

# Guess second part of artifact name using git repo name
GIT_REPO_NAME ?= $(shell XX=$$(git remote get-url origin); XX=$${XX\#\#*/};echo $${XX%%.git*})
# Default build number when running outside CI
BUILD_NUMBER ?= 1

# Get last commit ID for version generation
GIT_LAST_COMMIT_ID ?= $(shell git rev-parse --short HEAD)

# Get current branch for version generation
GIT_CURRENT_BRANCH ?= $(shell git rev-parse --abbrev-ref HEAD)

# Generate docker tag
DOCKER_TARGET_IMAGE_TAG  ?= $(BUILD_NUMBER)-$(subst /,-,$(GIT_CURRENT_BRANCH))-$(GIT_LAST_COMMIT_ID)

# Geenerate artifact/docker image name
DOCKER_TARGET_IMAGE_NAME ?= $(GIT_PROJECT)-$(GIT_REPO_NAME)

# Compile full docker image name including registry and tag
DOCKER_TARGET_IMAGE      ?= $(DOCKER_TARGET_REGISTRY)$(DOCKER_TARGET_IMAGE_NAME):$(DOCKER_TARGET_IMAGE_TAG)

# Default platform to build for 
TARGET_PLATFORM ?= rhel7
DOCKER_FILE_BUILD ?= src/Dockerfile.$(TARGET_PLATFORM) # Default path to dockerfile used to build resulting docker image in docker-build

# Default NEXUS source repository for dependency downloads
NEXUS_SOURCE_REPOSITORY_NAME ?= Sonatype Nexus Repository Manager
NEXUS_SOURCE_REPOSITORY_URL ?= https://nexus.transmit.im/repository/dialog
NEXUS_SOURCE_REPOSITORY_USERNAME ?= dialog
NEXUS_SOURCE_REPOSITORY_PASSWORD ?= dialog

# Default NEXUS target repository name for pushing artifacts
NEXUS_TARGET_REPOSITORY_NAME ?= Sonatype Nexus Repository Manager 
NEXUS_TARGET_REPOSITORY_URL ?= https://nexus.transmit.im/repository/dialog
NEXUS_TARGET_REPOSITORY_USERNAME ?= dialog
NEXUS_TARGET_REPOSITORY_PASSWORD ?= dialog

# Default NPM Source registry
NPM_SOURCE_REGISTRY ?= registry.npmjs.org
NPM_SOURCE_REGISTRY_TOKEN ?= $(NPM_TOKEN)

# Default NPM target registry
NPM_TARGET_REGISTRY ?= registry.npmjs.org
NPM_TARGET_REGISTRY_TOKEN ?= $(NPM_TOKEN)

# Default caches dir is user's home
GLOBAL_CACHES_DIR ?= ${HOME}

NPM_CACHE ?= $(GLOBAL_CACHES_DIR)/.npm
IVY2_CACHE ?= $(GLOBAL_CACHES_DIR)/.ivy2
SBT_CACHE ?= $(GLOBAL_CACHES_DIR)/.sbt
M2_CACHE ?= $(GLOBAL_CACHES_DIR)/.m2
COURSIER_CACHE ?= $(GLOBAL_CACHES_DIR)/.coursier/cache/v1
#COURSIER_CACHE ?= /Users/<user>/Library/Caches/Coursier/v1/https/ # for mac
DOCKER_RUNNER_HOME ?= /tmp
DOCKER_BUILD_WORKSPACE_SUBDIR ?= build
DOCKER_BUILD_WORKSPACE_DIR ?= $(shell realpath "$(shell git rev-parse --show-toplevel)/$(DOCKER_BUILD_WORKSPACE_SUBDIR)")
SBT_OPTS ?= -J-XX:+UseG1GC -J-Xmx4096M -Xms2048M -J-Xss2M -J-XX:MaxMetaspaceSize=1024M \
			-J-XX:+UseStringDeduplication 
SBT_OPTS += -J-Dsbt.global.base=$(DOCKER_RUNNER_HOME)/.sbt/ \
			-J-Dsbt.ivy.home=$(DOCKER_RUNNER_HOME)/.ivy2/ \
			-J-Divy.home=$(DOCKER_RUNNER_HOME)/.ivy2/ 


.PHONY: help
help:
	$(info $ Required: jdk-8, docker, sbt scala packager)
	$(info $ Minimal settings docker: 4 CPU, 4GB RAM)
	$(info $ DIALOG make usage:)
	$(info $ 	make test 		- Run app unit tests in TODO)
	$(info $ 	make docker-build	- Build app inside dockerfile and builds an image using the local Docker server)
	$(info $ 	docker-build-runner	- Build docker image for run targets make)
	$(info $ 	make build		- Build app in target root dir. No docker used)
	$(info $ 	make clean		- Remove all target dirs)
	$(info $ 	make docker-clean	- Remove target docker build)


define build-in-docker
	@echo [$@] Compiling code
	#time docker run -i --rm -u $(shell id -u):$(shell id -g) --entrypoint="/bin/bash" \
	-v "${NEXUS_CREDENTIALS}/":"$(DOCKER_RUNNER_HOME)/.m2":cached \
	-v "${IVY2_CACHE}/":"$(DOCKER_RUNNER_HOME)/.ivy2":cached \
	-v "${shell pwd}/":"$(DOCKER_RUNNER_HOME)/${GIT_REPO_NAME}":cached \
	-v "${SBT_CACHE}/":"$(DOCKER_RUNNER_HOME)/.sbt":cached \
	-v "${COURSIER_CACHE}":"$(DOCKER_RUNNER_HOME)/.cache/coursier/v1":cached \
	-v "${NPM_CACHE}/":"/tmp/.npm":cached  \
	-v "${M2_CACHE}/":"/tmp/.m2":cached  \
	-e HOME="$(DOCKER_RUNNER_HOME)" \
	-e BINTRAY_USER="$(BINTRAY_USER)" \
	-e BINTRAY_PASS="$(BINTRAY_PASS)" \
	-e NPM_TOKEN="$(NPM_SOURCE_REGISTRY_TOKEN)"  \
	-e NPM_REGISTRY="$(NPM_SOURCE_REGISTRY)"  \
	-e NEXUS_USERNAME="$(NEXUS_SOURCE_REPOSITORY_USERNAME)" \
	-e NEXUS_PASSWORD="$(NEXUS_SOURCE_REPOSITORY_PASSWORD)" \
	-e NEXUS_REPOSITORY_NAME="$(NEXUS_SOURCE_REPOSITORY_NAME)" \
	-e NEXUS_REPOSITORY_URL="$(NEXUS_SOURCE_REPOSITORY_URL)" \
	-e NEXUS_SOURCE_REPOSITORY_USERNAME="$(NEXUS_SOURCE_REPOSITORY_USERNAME)" \
	-e NEXUS_SOURCE_REPOSITORY_PASSWORD="$(NEXUS_SOURCE_REPOSITORY_PASSWORD)" \
	-e NEXUS_SOURCE_REPOSITORY_NAME="$(NEXUS_SOURCE_REPOSITORY_NAME)" \
	-e NEXUS_TARGET_REPOSITORY_URL="$(NEXUS_TARGET_REPOSITORY_URL)" \
	-e NEXUS_TARGET_REPOSITORY_USERNAME="$(NEXUS_TARGET_REPOSITORY_USERNAME)" \
	-e NEXUS_TARGET_REPOSITORY_PASSWORD="$(NEXUS_TARGET_REPOSITORY_PASSWORD)" \
	-e NEXUS_TARGET_REPOSITORY_NAME="$(NEXUS_TARGET_REPOSITORY_NAME)" \
	-e NEXUS_TARGET_REPOSITORY_URL="$(NEXUS_TARGET_REPOSITORY_URL)" \
	-e NPM_SOURCE_REGISTRY_TOKEN="$(NPM_SOURCE_REGISTRY_TOKEN)"  \
	-e NPM_SOURCE_REGISTRY="$(NPM_SOURCE_REGISTRY)"  \
	-e NPM_TARGET_REGISTRY_TOKEN="$(NPM_TARGET_REGISTRY_TOKEN)"  \
	-e NPM_TARGET_REGISTRY="$(NPM_TARGET_REGISTRY)"  \
	"${DOCKER_RUNNER_IMAGE}" -c "cd $(DOCKER_RUNNER_HOME)/${GIT_REPO_NAME} && bash && $1"
	@echo [$@] Compiling code completed!
endef

.PHONY: docker-build
docker-build:
ifeq "$(shell docker images -q $(DOCKER_TARGET_IMAGE))" ""
	@echo [$@] Compiling code and building docker image
	#docker build -f $(DOCKER_FILE_BUILD) -t $(DOCKER_TARGET_IMAGE) target/docker/stage/
	@echo [$@] Docker image $(DOCKER_TARGET_IMAGE) built successfully
else
	@echo [$@] Docker image $(DOCKER_TARGET_IMAGE) is already built. Run "make docker-clean" to wipe it
endif

.PHONY: docker-clean
docker-clean:
	@echo [$@] Remove docker image
	docker rmi -f $(DOCKER_TARGET_IMAGE) 
	docker rmi -f ${DOCKER_RUNNER}
	@echo [$@] Remove docker image completed!

.PHONY: docker-push
docker-push:
	docker push $(DOCKER_TARGET_IMAGE)


.PHONY: makefile-inc-update
makefile-inc-update:
	curl https://raw.githubusercontent.com/vsmirn0v/makelib/master/makefile.inc -OL
