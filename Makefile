-include PrivateRules.mak

PROJECT_NAME=ska-mid-cbf-fodm-gen

# W503: "Line break before binary operator." Disabled to work around a bug in flake8 where currently both "before" and "after" are disallowed.
PYTHON_SWITCHES_FOR_FLAKE8 = --ignore=DAR201,W503,E731

# F0002, F0010: Astroid errors. Not our problem.
# E1101: Class member not found. This breaks all classes with dynamic members.
PYTHON_SWITCHES_FOR_PYLINT = --disable=F0002,F0010,E1101
PYTHON_SWITCHES_FOR_PYLINT_LOCAL = --disable=F0002,F0010,E1101
PYTHON_LINE_LENGTH = 130
POETRY_PYTHON_RUNNER = poetry run python3 -m

PYTHON_LINT_TARGET = ./python/
PYTHON_VARS_AFTER_PYTEST = --forked

PYTHON_TEST_FILE = ./python/tests/

RELEASE_BUILD_DIR = ./build
DEBUG_BUILD_DIR = ./build-debug
ARMV8_BUILD_DIR = ./build-cross


include .make/*.mk


.PHONY: cpp-build cpp-build-debug cpp-build-armv8
cpp-build-release:
	rm -rf $(RELEASE_BUILD_DIR); mkdir $(RELEASE_BUILD_DIR); \
	cd $(RELEASE_BUILD_DIR); \
	conan install .. -pr ../profiles/default; \
	conan build ..

cpp-build-debug:
	rm -rf $(DEBUG_BUILD_DIR); mkdir $(DEBUG_BUILD_DIR); \
	cd $(DEBUG_BUILD_DIR); \
	conan install .. -pr ../profiles/default -s build_type=Debug; \
	conan build ..

cpp-build-armv8:
	rm -rf $(ARMV8_BUILD_DIR); mkdir $(ARMV8_BUILD_DIR); \
	cd $(ARMV8_BUILD_DIR); \
	conan install .. -pr:b ../profiles/default -pr:h ../profiles/armv8; \
	conan build ..

## OVERRIDE cicd makefile target: cpp-do-build
cpp-do-build: cpp-build-release cpp-build-debug cpp-build-armv8
	
cpp-test:
	@if [ ! -d $(DEBUG_BUILD_DIR) ]; then echo "Directory $(DEBUG_BUILD_DIR) does not exist. Ensure 'make cpp-build-debug' has been run first."; exit 1; fi;
	cd $(DEBUG_BUILD_DIR) && mkdir -p reports; \
	ctest --test-dir src/test --output-on-failure --force-new-ctest-process --output-junit reports/unit-tests.xml

cpp-clean:
	rm -rf $(RELEASE_BUILD_DIR)
	rm -rf $(DEBUG_BUILD_DIR)
	rm -rf $(ARMV8_BUILD_DIR)

format-python:
	$(POETRY_PYTHON_RUNNER) isort --profile black --line-length $(PYTHON_LINE_LENGTH) $(PYTHON_SWITCHES_FOR_ISORT) $(PYTHON_LINT_TARGET)
	$(POETRY_PYTHON_RUNNER) black --exclude .+\.ipynb --line-length $(PYTHON_LINE_LENGTH) $(PYTHON_SWITCHES_FOR_BLACK) $(PYTHON_LINT_TARGET)

lint-python-local:
	mkdir -p build/lint-output
	@echo "Linting..."
	-@ISORT_ERROR=0 BLACK_ERROR=0 FLAKE_ERROR=0 PYLINT_ERROR=0; \
	$(POETRY_PYTHON_RUNNER) isort --check-only --profile black --line-length $(PYTHON_LINE_LENGTH) $(PYTHON_SWITCHES_FOR_ISORT) $(PYTHON_LINT_TARGET) &> build/lint-output/1-isort-output.txt; \
	if [ $$? -ne 0 ]; then ISORT_ERROR=1; fi; \
	$(POETRY_PYTHON_RUNNER) black --exclude .+\.ipynb --check --line-length $(PYTHON_LINE_LENGTH) $(PYTHON_SWITCHES_FOR_BLACK) $(PYTHON_LINT_TARGET) &> build/lint-output/2-black-output.txt; \
	if [ $$? -ne 0 ]; then BLACK_ERROR=1; fi; \
	$(POETRY_PYTHON_RUNNER) flake8 --show-source --statistics --max-line-length $(PYTHON_LINE_LENGTH) $(PYTHON_SWITCHES_FOR_FLAKE8) $(PYTHON_LINT_TARGET) &> build/lint-output/3-flake8-output.txt; \
	if [ $$? -ne 0 ]; then FLAKE_ERROR=1; fi; \
	$(POETRY_PYTHON_RUNNER) pylint --output-format=parseable --max-line-length $(PYTHON_LINE_LENGTH) $(PYTHON_SWITCHES_FOR_PYLINT_LOCAL) $(PYTHON_LINT_TARGET) &> build/lint-output/4-pylint-output.txt; \
	if [ $$? -ne 0 ]; then PYLINT_ERROR=1; fi; \
	if [ $$ISORT_ERROR -ne 0 ]; then echo "Isort lint errors were found. Check build/lint-output/1-isort-output.txt for details."; fi; \
	if [ $$BLACK_ERROR -ne 0 ]; then echo "Black lint errors were found. Check build/lint-output/2-black-output.txt for details."; fi; \
	if [ $$FLAKE_ERROR -ne 0 ]; then echo "Flake8 lint errors were found. Check build/lint-output/3-flake8-output.txt for details."; fi; \
	if [ $$PYLINT_ERROR -ne 0 ]; then echo "Pylint lint errors were found. Check build/lint-output/4-pylint-output.txt for details."; fi; \
	if [ $$ISORT_ERROR -eq 0 ] && [ $$BLACK_ERROR -eq 0 ] && [ $$FLAKE_ERROR -eq 0 ] && [ $$PYLINT_ERROR -eq 0 ]; then echo "Lint was successful. Check build/lint-output for any additional details."; fi;
