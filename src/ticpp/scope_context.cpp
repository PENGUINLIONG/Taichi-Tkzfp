#include "ticpp/scope_context.hpp"
#include "ticpp/stmt.hpp"

namespace ticpp {

void ScopeContext::commit_stmt(const Stmt* stmt) {
  ss << std::string(indent, ' ');
  stmt->to_string(ss);
  ss << std::endl;
}

std::string ScopeContext::finish() {
  std::string out = ss.str();
  ss.clear();
  return out;
}

thread_local ScopeContext scope_context_;


} // namespace ticpp
