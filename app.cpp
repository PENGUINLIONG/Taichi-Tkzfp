#include <fstream>
#include "ticpp/expr.hpp"
#include "ticpp/stmt.hpp"
#include "ticpp/codegen.hpp"

namespace ticpp {






} // namespace ticpp

void kernel_impl(ticpp::ExprRef i, ticpp::ExprRef f, ticpp::ExprRef ndarray) {
  auto stmt = ticpp::StoreStmt::create(
    ticpp::IndexExpr::create(ndarray, ticpp::TupleExpr::create({ ticpp::IntImmExpr::create(0), i })), f);
  stmt->commit();
}

int main(int argc, const char** argv) {
  ti::Runtime runtime(TI_ARCH_VULKAN);

  ti::NdArray<float> arr = runtime.allocate_ndarray<float>({4, 8}, {2}, true);

  auto x = ticpp::to_kernel(kernel_impl);
  std::string code = x.compile(1, 1.23f, arr.ndarray());

  std::fstream f("app.py", std::ios::out | std::ios::trunc);
  f << code << std::endl;

  system("/Users/penguinliong/opt/anaconda3/bin/python3 app.py");

  ti::AotModule mod = runtime.load_aot_module("module");
  ti::ComputeGraph g = mod.get_compute_graph("g");
  g["_0"] = 1;
  g["_1"] = 1.23f;
  g["_2"] = arr.ndarray();

  g.launch();

  runtime.wait();

  std::vector<float> host_arr(4 * 8 * 2);
  arr.read(host_arr);

  for (const float& x : host_arr) {
    std::cout << x << std::endl;
  }

  return 0;
}
