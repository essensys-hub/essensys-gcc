# Makefile — Top-level build for Essensys GCC firmware
#
# Targets:
#   make build-bp           Compile BP (SC944D) with VERSION support
#   make build-ba           Compile BA boards (SC940, SC941C, SC942C)
#   make test              Run all unit tests (BP, BA, integration)
#   make test-coverage     Run tests + generate code coverage (LCOV)
#   make coverage          Generate code coverage report (requires prior test run)
#   make misra             Run MISRA C analysis (cppcheck)
#   make all               Compile BP + BA
#   make clean             Remove build artifacts

VERSION  ?= dev

.PHONY: all build-bp build-ba test test-coverage coverage misra clean

all: build-bp build-ba

# === BUILD TARGETS ===

build-bp:
	$(MAKE) -C bp VERSION=$(VERSION) build

build-ba:
	$(MAKE) -C ba all-boards

# Backward compatible targets
bp: build-bp
ba: build-ba

# === TEST TARGETS ===

test: test-bp test-ba test-integration

test-bp:
	$(MAKE) -C tests/bp run_tests

test-ba:
	$(MAKE) -C tests/ba run_tests

test-integration:
	$(MAKE) -C tests/integration run_tests

# === COVERAGE TARGETS ===

test-coverage: clean-coverage test coverage
	@echo "✓ Coverage report: coverage/index.html"

coverage:
	@echo "Generating LCOV coverage report..."
	@mkdir -p coverage
	@find build/bp/tests -name "*.gcda" -o -name "*.gcno" | xargs lcov -c -d . -o coverage/coverage.lcov
	@lcov --remove coverage/coverage.lcov '/usr/*' -o coverage/coverage.lcov
	@genhtml coverage/coverage.lcov -o coverage/
	@grep -oP 'lines......: \K[0-9.]+' coverage/index.html | head -1 > coverage/summary.txt || echo "0" > coverage/summary.txt
	@echo "Coverage: $$(cat coverage/summary.txt)%"

clean-coverage:
	rm -rf coverage/

# === STATIC ANALYSIS ===

misra:
	@echo "Running MISRA C analysis (cppcheck)..."
	@mkdir -p reports
	@cppcheck --enable=all --std=c99 --suppress=missingIncludeSystem \
	  --addon=misra --addon-python-path=/usr/bin/python3 \
	  --xml --xml-version=2 bp/C/*.c 2> reports/misra.xml
	@python3 -c "import xml.etree.ElementTree as ET; tree=ET.parse('reports/misra.xml'); violations=[v for v in tree.findall('.//error')]; print(f'Found {len(violations)} MISRA violations')"
	@cp reports/misra.xml reports/misra-report.html || true
	@echo "✓ MISRA report: reports/misra-report.html"

# === CLEANUP ===

clean:
	$(MAKE) -C bp clean
	$(MAKE) -C ba clean
	rm -rf coverage/ reports/
