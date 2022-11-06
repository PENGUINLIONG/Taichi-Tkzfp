#include <fstream>
#include "ticpp/expr.hpp"
#include "ticpp/stmt.hpp"
#include "ticpp/codegen.hpp"

namespace ticpp {





} // namespace ticpp

void kernel_impl(ticpp::IntValue i, ticpp::FloatValue f, ticpp::NdArrayValue ndarray) {
  ndarray[{0, i}] = f;
}

int main(int argc, const char** argv) {
  ti::Runtime runtime(TI_ARCH_VULKAN);

  ti::NdArray<float> arr = runtime.allocate_ndarray<float>({4, 8}, {2}, true);

  auto x = ticpp::to_kernel(runtime, kernel_impl);
  x.launch(1, 1.23f, arr.ndarray());

  runtime.wait();

  std::vector<float> host_arr(4 * 8 * 2);
  arr.read(host_arr);
  std::cout << host_arr.at(2) << std::endl;

  //for (const float& x : host_arr) {
  //  std::cout << x << std::endl;
  //}

  return 0;
}
