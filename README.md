# TestGen: Specification-Based API Test Generation

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Z3](https://img.shields.io/badge/Z3-SMT%20Solver-green.svg)](https://github.com/Z3Prover/z3)

**TestGen** is a specification-based test generation framework for REST APIs that uses symbolic execution and SMT constraint solving to automatically generate concrete, executable test cases from high-level formal specifications.

## Overview

Unlike schema-based tools (Schemathesis, RESTler) that test APIs in isolation, TestGen reasons about **multi-step stateful workflows** involving:

- Authentication and role-based access control
- Cross-endpoint entity dependencies
- Complex state machines and business logic

TestGen takes a formal API specification written in a custom DSL and produces executable test cases that exercise realistic end-to-end scenarios.

## Key Features

- **Symbolic Execution Engine**: Tracks symbolic state across API call sequences
- **Z3 SMT Solver Integration**: Generates concrete values satisfying path constraints
- **Multi-Actor Support**: Handles role-based workflows (e.g., Seller→Buyer, Owner→Customer→Agent)
- **Authentication Management**: Automatically manages JWT tokens across role transitions
- **Entity Dependency Resolution**: Resolves dynamic IDs and cross-endpoint references
- **Infeasibility Detection**: Identifies and prunes logically impossible test sequences (UNSAT)
- **Live Backend Execution**: Executes generated tests against running applications

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        TestGen Framework                         │
├─────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐      │
│  │    Spec      │    │   Symbolic   │    │     Z3       │      │
│  │   Parser     │───▶│   Executor   │───▶│   Solver     │      │
│  └──────────────┘    └──────────────┘    └──────────────┘      │
│         │                   │                   │               │
│         ▼                   ▼                   ▼               │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐      │
│  │  Test String │    │  Constraint  │    │   Concrete   │      │
│  │   Parsing    │    │ Accumulation │    │    Values    │      │
│  └──────────────┘    └──────────────┘    └──────────────┘      │
│                             │                                   │
│                             ▼                                   │
│                    ┌──────────────┐                             │
│                    │  HTTP Test   │                             │
│                    │  Execution   │                             │
│                    └──────────────┘                             │
└─────────────────────────────────────────────────────────────────┘
```

## Prerequisites

- **C++17** compatible compiler (g++ 9+ or clang++ 10+)
- **Z3 SMT Solver** (v4.8+)
- **libcurl** for HTTP requests
- **nlohmann/json** for JSON parsing
- **CMake** 3.16+ (optional, for build)

### Installing Dependencies

**Ubuntu/Debian:**

```bash
sudo apt-get update
sudo apt-get install -y build-essential libz3-dev libcurl4-openssl-dev nlohmann-json3-dev cmake
```

**macOS (Homebrew):**

```bash
brew install z3 curl nlohmann-json cmake
```

**Windows (vcpkg):**

```bash
vcpkg install z3:x64-windows curl:x64-windows nlohmann-json:x64-windows
```

## Installation

```bash
# Clone the repository
git clone <repository-url>
cd TestgenTool

# Build the project
make

# Run the application
make run
```

### Build Commands

| Command        | Description               |
| -------------- | ------------------------- |
| `make`         | Build the project         |
| `make run`     | Build and run the project |
| `make clean`   | Remove built files        |
| `make rebuild` | Clean and rebuild         |
| `make help`    | Show help message         |

### Platform Configuration

The Makefile is configured for **macOS (Homebrew)** by default. For Linux, edit the `Makefile` and uncomment the Linux paths:

```makefile
# Linux (uncomment if using Linux)
INCLUDES = -I/usr/include
LDFLAGS = -L/usr/lib
```

### Manual Build (Alternative)

```bash
g++ -std=c++17 -Wall -I. -Isee -Itester -Ispecs \
    -I/opt/homebrew/include \
    test_libapplication.cpp \
    algo.cpp ast.cc astvisitor.cc printvisitor.cc \
    clonevisitor.cc rewrite_globals_visitor.cc \
    symvar.cc env.cc typemap.cc \
    specs/RestaurantSpec.cpp \
    specs/EcommerceSpec.cpp \
    specs/LibrarySpec.cpp \
    see/see.cc see/solver.cc see/z3solver.cc \
    see/functionfactory.cc see/httpclient.cc \
    see/restaurantfunctionfactory.cc \
    see/ecommercefunctionfactory.cc \
    see/libraryfunctionfactory.cc \
    tester/tester.cc \
    -L/opt/homebrew/lib \
    -lz3 -lcurl \
    -o test_libapplication

./test_libapplication
```

**Note:** For Linux, change `/opt/homebrew` to `/usr` in the include and library paths.

## Usage

### Running Test Cases

1. **Start the target web application backend** (the API you want to test must be running)

2. **Select the test string** to run by editing `test_libapplication.cpp`:
   - Open `test_libapplication.cpp`
   - In the `main()` function, uncomment the desired test string
   - Comment out other test strings

3. **Build and run:**
   ```bash
   make run
   ```

### Test Strings by Application

| Application                | Test Strings Location |
| -------------------------- | --------------------- |
| Restaurant (Food Ordering) | Uncomment in `main()` |
| E-commerce                 | Uncomment in `main()` |
| Library Management         | Uncomment in `main()` |

### Test Output Files

Test outputs are stored in the `all_tests_files/` folder:

| Application                | File Prefix     |
| -------------------------- | --------------- |
| Restaurant (Food Ordering) | `test*.txt`     |
| E-commerce                 | `test*ecom.txt` |
| Library Management         | `lib*.txt`      |

### Backend Requirements

Before running tests, ensure the corresponding backend is running:

| Application                | Default URL             |
| -------------------------- | ----------------------- |
| Restaurant (Food Ordering) | `http://localhost:9000` |
| E-commerce                 | `http://localhost:3000` |
| Library Management         | `http://localhost:8080` |

## Writing Specifications

TestGen uses a formal specification language to define API endpoints, their preconditions, postconditions, and state effects.

### Specification Structure

```cpp
// EcommerceSpec.cpp

// Define symbolic variables
SymbolicVar sellerEmail("sellerEmail", Type::String);
SymbolicVar sellerPassword("sellerPassword", Type::String);
SymbolicVar sellerToken("sellerToken", Type::String);
SymbolicVar productId("productId", Type::String);

// Define API blocks
Block registerSeller {
    .name = "registerSeller",
    .method = "POST",
    .endpoint = "/api/auth/register",
    .body = {
        {"email", sellerEmail},
        {"password", sellerPassword},
        {"userType", "Seller"}
    },
    .precondition = Not(Exists("users", sellerEmail)),
    .postcondition = And(
        Exists("users", sellerEmail),
        HasRole(sellerEmail, "Seller")
    )
};

Block loginSeller {
    .name = "loginSeller",
    .method = "POST",
    .endpoint = "/api/auth/login",
    .body = {
        {"email", sellerEmail},
        {"password", sellerPassword}
    },
    .precondition = Exists("users", sellerEmail),
    .postcondition = ValidToken(sellerToken),
    .extracts = {{"token", sellerToken}}
};

Block createProduct {
    .name = "createProduct",
    .method = "POST",
    .endpoint = "/api/products",
    .headers = {{"Authorization", Concat("Bearer ", sellerToken)}},
    .body = {
        {"name", SymbolicVar("productName", Type::String)},
        {"price", SymbolicVar("productPrice", Type::Int)}
    },
    .precondition = And(ValidToken(sellerToken), HasRole(sellerEmail, "Seller")),
    .postcondition = Exists("products", productId),
    .extracts = {{"_id", productId}}
};
```

### Supported Constraint Types

| Constraint                | Description                    |
| ------------------------- | ------------------------------ |
| `Exists(collection, id)`  | Entity exists in collection    |
| `Not(constraint)`         | Logical negation               |
| `And(c1, c2, ...)`        | Logical conjunction            |
| `Or(c1, c2, ...)`         | Logical disjunction            |
| `HasRole(user, role)`     | User has specified role        |
| `ValidToken(token)`       | Token is valid and not expired |
| `Equals(var, value)`      | Variable equals value          |
| `GreaterThan(var, value)` | Numeric comparison             |
| `InState(entity, state)`  | Entity is in specified state   |

## Project Structure

```
TestgenTool/
│
├── Makefile                    # Build configuration
├── test_libapplication.cpp     # Main entry point (comprises all tests)
├── algo.cpp                    # genATC algorithm implementation
├── algo.hpp
├── ast.cc                      # Abstract Syntax Tree implementation
├── ast.hh
├── astvisitor.cc               # AST visitor pattern implementation
├── astvisitor.hh
├── printvisitor.cc             # Print visitor for AST
├── printvisitor.hh
├── clonevisitor.cc             # Clone visitor for AST
├── clonevisitor.hh
├── rewrite_globals_visitor.cc  # Rewrite globals visitor (test API layer)
├── rewrite_globals_visitor.hh
├── env.cc                      # Environment/state management
├── env.hh
├── typemap.cc                  # Type mapping utilities
├── typemap.hh
├── symvar.cc                   # Symbolic variable implementation
├── symvar.hh
│
├── specs/                      # API Specification files
│   ├── RestaurantSpec.cpp      # Food Ordering API specification
│   ├── RestaurantSpec.hpp
│   ├── EcommerceSpec.cpp       # E-commerce API specification
│   ├── EcommerceSpec.hpp
│   ├── LibrarySpec.cpp         # Library Management API specification
│   └── LibrarySpec.hpp
│
├── tester/                     # Test generation module
│   ├── tester.cc               # Test case generator (genCTC, etc.)
│   └── tester.hh
│
└── see/                        # Symbolic Execution Engine
    ├── see.cc                  # Symbolic execution engine core
    ├── see.hh
    ├── solver.cc               # Abstract solver interface
    ├── solver.hh
    ├── z3solver.cc             # Z3 SMT solver integration
    ├── z3solver.hh
    ├── functionfactory.cc      # Base function factory
    ├── functionfactory.hh
    ├── restaurantfunctionfactory.cc   # Restaurant app functions
    ├── restaurantfunctionfactory.hh
    ├── ecommercefunctionfactory.cc    # E-commerce app functions
    ├── ecommercefunctionfactory.hh
    ├── libraryfunctionfactory.cc      # Library app functions
    ├── libraryfunctionfactory.hh
    ├── httpclient.cc           # HTTP client for live execution
    └── httpclient.hh

├── all_tests_files/            # Test output files
│   ├── test*.txt              # Restaurant (Food Ordering) test outputs
│   ├── test*ecom.txt          # E-commerce test outputs
│   └── lib*.txt               # Library Management test outputs
```

### Module Descriptions

| Module                      | Description                                                        |
| --------------------------- | ------------------------------------------------------------------ |
| **Makefile**                | Build configuration for the project                                |
| **test_libapplication.cpp** | Main entry point that runs all test cases                          |
| **algo**                    | genATC algorithm - generates Abstract Test Cases from test strings |
| **ast**                     | Abstract Syntax Tree for specification parsing                     |
| **printvisitor**            | Print visitor for AST visualization                                |
| **clonevisitor**            | Clone visitor for AST duplication                                  |
| **rewrite_globals_visitor** | Rewrite globals visitor for test API layer                         |
| **env**                     | Environment and state management across API calls                  |
| **symvar**                  | Symbolic variable definitions and operations                       |
| **specs/**                  | Formal API specifications for each subject application             |
| **tester/**                 | Concrete Test Case (CTC) generation from symbolic results (genCTC) |
| **see/**                    | Symbolic Execution Engine with Z3 solver and HTTP client           |
| **all_tests_files/**        | Test output files for all three applications                       |

## Supported Applications

TestGen has been evaluated on three real-world web applications:

| Application        | Endpoints | Roles                  | Test String Length |
| ------------------ | --------- | ---------------------- | ------------------ |
| E-commerce         | 15        | Seller, Buyer          | 15                 |
| Food Ordering      | 19        | Owner, Customer, Agent | 19                 |
| Library Management | 10        | Admin, Student         | 10                 |

## Comparison with Other Tools

| Feature                      | TestGen | RESTler | Schemathesis |
| ---------------------------- | ------- | ------- | ------------ |
| Multi-step workflows         | Yes     | No      | No           |
| Authentication management    | Yes     | No      | No           |
| Role-based testing           | Yes     | No      | No           |
| Entity dependency resolution | Yes     | Partial | No           |
| Symbolic execution           | Yes     | No      | No           |
| Constraint solving           | Yes     | No      | No           |
| Infeasibility detection      | Yes     | No      | No           |
| Schema-based fuzzing         | No      | Yes     | Yes          |

### OMTSP (Observed Maximum Test String Penetration)

| Application   | TestGen | RESTler | Schemathesis |
| ------------- | ------- | ------- | ------------ |
| E-commerce    | 1.00    | 0.07    | 0.13         |
| Food Ordering | 1.00    | 0.05    | 0.05         |
| Library       | 1.00    | 0.30    | 0.30         |

## Example Output

```
$ make run

[TestGen] Loading E-commerce Specification...
[TestGen] Test string: registerSeller -> loginSeller -> createProduct -> registerBuyer -> loginBuyer -> addToCart -> placeOrder
[TestGen] Starting symbolic execution...

[Step 1/7] registerSeller
  Generating constraints...
  Solving with Z3... SAT
  Generated values: email = "seller_test@example.com", password = "Pass123!"

[Step 2/7] loginSeller
  Precondition: User exists
  Solving with Z3... SAT
  Extracted: token = "eyJhbGciOiJIUzI1NiIs..."

[Step 3/7] createProduct
  Precondition: Valid seller token
  Solving with Z3... SAT
  Generated: productName = "TestProduct", price = 99
  Extracted: productId = "507f1f77bcf86cd799439011"

...

[TestGen] Executing HTTP requests against http://localhost:3000...

POST /api/auth/register -> 201 Created
POST /api/auth/login -> 200 OK
POST /api/products -> 201 Created
POST /api/auth/register -> 201 Created
POST /api/auth/login -> 200 OK
POST /api/cart -> 200 OK
POST /api/orders -> 201 Created

[TestGen] Test execution complete. All 7 steps passed.
```

## Troubleshooting

### Common Issues

**Z3 not found:**

```bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

**Connection refused:**
Ensure the target application is running on the specified base URL.

**UNSAT result:**
The test string represents an infeasible scenario. Check preconditions.

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Acknowledgments

- [Z3 Theorem Prover](https://github.com/Z3Prover/z3) by Microsoft Research
- [nlohmann/json](https://github.com/nlohmann/json) for JSON parsing
- [libcurl](https://curl.se/libcurl/) for HTTP requests
