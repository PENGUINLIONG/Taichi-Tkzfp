#include "ticpp/expr.hpp"
#include "ticpp/stmt.hpp"
#include "ticpp/codegen.hpp"

namespace ticpp {






} // namespace ticpp

void kernel_impl(ticpp::ExprRef i, ticpp::ExprRef f, ticpp::ExprRef ndarray) {
  auto stmt = ticpp::StoreStmt::create(
    ticpp::IndexExpr::create(ndarray, ticpp::TupleExpr::create({ i, i })), f);
  stmt->commit();
}

int main(int argc, const char** argv) {
  TiNdArray nd{};
  auto x = ticpp::to_kernel(kernel_impl);
  std::string code = x.compile(1, 1.23f, nd);
  std::cout << code << std::endl;

  return 0;
}
