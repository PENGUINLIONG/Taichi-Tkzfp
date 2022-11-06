#include <fstream>
#include "ticpp/expr.hpp"
#include "ticpp/stmt.hpp"
#include "ticpp/codegen.hpp"

namespace ticpp {





} // namespace ticpp

void kernel_impl(ticpp::IntValue i, ticpp::FloatValue f, ticpp::NdArrayValue ndarray) {
  TICPP_FOR(idxs, ndarray) {
    ndarray[idxs] = ticpp::VectorValue({
      ticpp::to_float(idxs[0]),
      ticpp::to_float(idxs[1])
    });
  };
}

int main(int argc, const char** argv) {
  ti::Runtime runtime(TI_ARCH_VULKAN);

  ti::NdArray<float> arr = runtime.allocate_ndarray<float>({4, 8}, {2}, true);

  auto x = ticpp::to_kernel(runtime, kernel_impl);
  x.launch(1, 1.23f, arr.ndarray());

  runtime.wait();

  std::vector<float> host_arr(4 * 8 * 2);
  arr.read(host_arr);

  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 8; ++j) {
      std::cout << "(";
      for (size_t k = 0; k < 2; ++k) {
        std::cout << host_arr[(i * 8 + j) * 2 + k];
      }
      std::cout << ")";
    }
    std::cout << std::endl;
  }

  // for (const float& x : host_arr) {
  //   std::cout << x << std::endl;
  // }

  return 0;
}
