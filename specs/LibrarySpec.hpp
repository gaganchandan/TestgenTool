#pragma once

#include "../language/ast.hh"
#include <memory>
#include <vector>

std::unique_ptr<Spec> makeLibrarySpec();
