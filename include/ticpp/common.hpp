// Common includes.
// @PENGUINLIONG
#pragma once
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <functional>
#include <taichi/cpp/taichi.hpp>

namespace ticpp {

struct PythonScriptWriter {
  std::string indent;
  std::stringstream ss;

  void push_indent() {
    indent += "    ";
    ss << "    ";
  }
  void pop_indent() {
    indent.resize(indent.size() - 4);
    commit_line();
  }
  void commit_line() {
    ss << std::endl << indent;
  }
  void clear() {
    ss.clear();
  }
  std::string str() {
    return ss.str();
  }

  template<typename T>
  PythonScriptWriter& operator <<(const T& x) {
    ss << x;
    return *this;
  }
};

}
