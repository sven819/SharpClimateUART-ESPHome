#!/bin/bash

set -e  # Exit on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}"
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║         Sharp AC Component - Test Suite Runner               ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Function to display step headers
step() {
    echo -e "\n${YELLOW}>>> $1${NC}"
}

# Function to display success
success() {
    echo -e "${GREEN}✓ $1${NC}"
}

# Function to display error
error() {
    echo -e "${RED}✗ $1${NC}"
}

# Check if g++ is installed
if ! command -v g++ &> /dev/null; then
    error "g++ is not installed. Please install it first."
    exit 1
fi

success "g++ found: $(g++ --version | head -n1)"

# Check if make is installed
if ! command -v make &> /dev/null; then
    error "make is not installed. Please install it first."
    exit 1
fi

success "make found: $(make --version | head -n1)"

# Clean
step "Cleaning old build artifacts..."
make -f Makefile.test clean > /dev/null 2>&1
success "Cleanup complete"

# Build
step "Compiling test suites..."
if make -f Makefile.test; then
    success "Compilation successful"
else
    error "Compilation failed"
    exit 1
fi

# Counter for test results
TOTAL_TESTS=0
PASSED_TESTS=0

# Run Frame Parsing Tests
step "Running frame parsing tests..."
echo ""
if ./test_frame_parsing; then
    success "Frame parsing tests passed"
    ((PASSED_TESTS++))
else
    error "Frame parsing tests failed"
fi
((TOTAL_TESTS++))

# Run Core Logic Tests
step "Running core logic tests..."
echo ""
if ./test_core_logic; then
    success "Core logic tests passed"
    ((PASSED_TESTS++))
else
    error "Core logic tests failed"
fi
((TOTAL_TESTS++))

# Run Integration Tests
step "Running integration tests..."
echo ""
if ./test_integration; then
    success "Integration tests passed"
    ((PASSED_TESTS++))
else
    error "Integration tests failed"
fi
((TOTAL_TESTS++))

# Summary
echo -e "\n${BLUE}"
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                      Test Summary                            ║"
echo "╠══════════════════════════════════════════════════════════════╣"
printf "║  Test suites passed: %d / %d                                    ║\n" $PASSED_TESTS $TOTAL_TESTS
echo "╚══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    echo -e "${GREEN}"
    echo "✓✓✓ All test suites passed! ✓✓✓"
    echo -e "${NC}"
    exit 0
else
    echo -e "${RED}"
    echo "✗✗✗ Some test suites failed! ✗✗✗"
    echo ""
    echo "Please check the detailed output above."
    echo -e "${NC}"
    exit 1
fi
