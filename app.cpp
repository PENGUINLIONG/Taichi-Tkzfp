#include <fstream>
#include "ticpp/expr.hpp"
#include "ticpp/stmt.hpp"
#include "ticpp/codegen.hpp"

namespace ticpp {

struct IntValue {
  ExprRef expr_;

  IntValue(int value) : expr_(IntImmExpr::create(value)) {}
  IntValue(ExprRef&& expr) : expr_(std::move(expr)) {}

  friend IntValue operator+(const IntValue& a, const IntValue& b) {
    return IntValue { AddExpr::create(a.expr_, b.expr_) };
  }
  friend IntValue operator-(const IntValue& a, const IntValue& b) {
    return IntValue { SubExpr::create(a.expr_, b.expr_) };
  }
};
struct FloatValue {
  ExprRef expr_;

  FloatValue(float value) : expr_(FloatImmExpr::create(value)) {}
  FloatValue(ExprRef&& expr) : expr_(std::move(expr)) {}

  friend FloatValue operator+(const FloatValue& a, const FloatValue& b) {
    return FloatValue { AddExpr::create(a.expr_, b.expr_) };
  }
};
struct TupleValue {
  ExprRef expr_;

  TupleValue(std::vector<ExprRef>&& exprs) : expr_(TupleExpr::create(std::move(exprs))) {}
};
struct NdArrayValue {
  ExprRef expr_;

  NdArrayValue(const std::string& name, const TiNdArray& ndarray) :
    expr_(NdArrayAllocExpr::create(name, ndarray)) {}
  NdArrayValue(ExprRef&& expr) : expr_(std::move(expr)) {}

  NdArrayValue operator[](const std::vector<IntValue>& idxs) {
    std::vector<ExprRef> idxs2(idxs.size());
    for (size_t i = 0; i < idxs.size(); ++i) {
      idxs2.at(i) = idxs.at(i).expr_;
    }
    return NdArrayValue { IndexExpr::create(expr_, TupleExpr::create(std::move(idxs2))) };
  }

  NdArrayValue& operator=(const IntValue& x) {
    StoreStmt::create(expr_, x.expr_)->commit();
    return *this;
  }
  NdArrayValue& operator=(const FloatValue& x) {
    StoreStmt::create(expr_, x.expr_)->commit();
    return *this;
  }
};





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
  std::cout << host_arr.at(2) << std::endl;

  //for (const float& x : host_arr) {
  //  std::cout << x << std::endl;
  //}

  return 0;
}
