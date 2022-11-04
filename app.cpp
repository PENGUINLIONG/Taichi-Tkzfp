#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <functional>
#include <taichi/cpp/taichi.hpp>

namespace ticpp {

const char* arch2str(TiArch arch) {
  switch (arch) {
  case TI_ARCH_VULKAN:
    return "ti.vulkan";
  default:
    assert(false);
  }
  return "#";
}

const char* dtype2str(TiDataType dtype) {
  switch (dtype) {
  case TI_DATA_TYPE_F16:
    return "ti.f16";
  case TI_DATA_TYPE_F32:
    return "ti.f32";
  case TI_DATA_TYPE_F64:
    return "ti.f64";
  case TI_DATA_TYPE_I8:
    return "ti.i8";
  case TI_DATA_TYPE_I16:
    return "ti.i16";
  case TI_DATA_TYPE_I32:
    return "ti.i32";
  case TI_DATA_TYPE_I64:
    return "ti.i64";
  case TI_DATA_TYPE_U8:
    return "ti.u8";
  case TI_DATA_TYPE_U16:
    return "ti.u16";
  case TI_DATA_TYPE_U32:
    return "ti.u32";
  case TI_DATA_TYPE_U64:
    return "ti.u64";
  default:
    assert(false);
  }
  return "#";
}

std::string shape2str(const TiNdShape& shape) {
  std::stringstream ss;
  ss << "(";
  if (shape.dim_count != 0) {
    for (uint32_t i = 0; i < shape.dim_count; ++i) {
      ss << shape.dims[i] << ",";
    }
  } else {
    ss << ",";
  }
  ss << ")";
  return ss.str();
}

struct NamedArgument {
  std::string arg_name;
  TiNamedArgument arg;
};
typedef std::unique_ptr<NamedArgument> NamedArgumentRef;

std::string build_symbols(const std::vector<NamedArgumentRef>& args) {
  std::stringstream ss;
  for (const NamedArgumentRef& arg2 : args) {
    const TiNamedArgument& arg = arg2->arg;

    ss << "sym" << arg.name << " = ";

    switch (arg.argument.type) {
    case TI_ARGUMENT_TYPE_I32:
      ss << "ti.graph.Arg(ti.graph.ArgKind.SCALAR,'" << arg.name << "',ti.i32)";
      break;
    case TI_ARGUMENT_TYPE_F32:
      ss << "ti.graph.Arg(ti.graph.ArgKind.SCALAR,'" << arg.name << "',ti.f32)";
      break;
    case TI_ARGUMENT_TYPE_NDARRAY:
      ss << "ti.graph.Arg(ti.graph.ArgKind.NDARRAY,"
        << "'" << arg.name << "',"
        << dtype2str(arg.argument.value.ndarray.elem_type) << ","
        << "field_dim=" << arg.argument.value.ndarray.shape.dim_count << ","
        << "element_shape=" << shape2str(arg.argument.value.ndarray.elem_shape) << ")";
      break;
    default:
      assert(false);
    }
    ss << std::endl;
  }
  return ss.str();
}
std::string build_params(const std::vector<NamedArgumentRef>& args) {
  std::stringstream ss;
  for (const NamedArgumentRef& arg2 : args) {
    const TiNamedArgument& arg = arg2->arg;

    ss << arg.name << ":";

    switch (arg.argument.type) {
    case TI_ARGUMENT_TYPE_I32:
      ss << "ti.i32";
      break;
    case TI_ARGUMENT_TYPE_F32:
      ss << "ti.f32";
      break;
    case TI_ARGUMENT_TYPE_NDARRAY:
      ss << "ti.ndarray(field_dim=" << arg.argument.value.ndarray.shape.dim_count << ")";
      break;
    default:
      assert(false);
    }
    ss << ",";
  }
  return ss.str();
}
std::string build_args(const std::vector<NamedArgumentRef>& args) {
  std::stringstream ss;
  for (const NamedArgumentRef& arg : args) {
    ss << "sym" << arg->arg_name << ",";
  }
  return ss.str();
}

std::string dump_taichi_aot_script(TiArch arch, const std::vector<NamedArgumentRef>& args) {
  std::stringstream ss;
  ss << R"(
import taichi as ti

ti.init()" << arch2str(arch) << R"()

)" << build_symbols(args) << R"(

@ti.kernel
def f()" << build_params(args) << R"():
    pass

g_builder = ti.graph.GraphBuilder()
g_builder.dispatch(f,)" << build_args(args) << R"()
graph = g_builder.compile()

mod = ti.aot.Module()" << arch2str(arch) << R"()
mod.add_graph('g', graph)
mod.save("module", '')
)";
  return ss.str();
}









template<typename T>
struct get_arg_ti_arg {};
template<>
struct get_arg_ti_arg<int32_t> {
  static inline void get(TiArgument& arg, int32_t value) {
    arg.type = TI_ARGUMENT_TYPE_I32;
    arg.value.i32 = value;
  }
};
template<>
struct get_arg_ti_arg<float> {
  static inline void get(TiArgument& arg, float value) {
    arg.type = TI_ARGUMENT_TYPE_F32;
    arg.value.f32 = value;
  }
};
template<>
struct get_arg_ti_arg<TiNdArray> {
  static inline void get(TiArgument& arg, const TiNdArray& value) {
    arg.type = TI_ARGUMENT_TYPE_NDARRAY;
    arg.value.ndarray = value;
  }
};



template<typename ... TArgs>
struct collect_args {};
template<typename TFirst, typename ... TArgs>
struct collect_args<TFirst, TArgs ...> {
  static inline void collect(std::vector<NamedArgumentRef>& out, const TFirst& x, const TArgs& ... args) {
    collect_args<TFirst>::collect(out, x);
    collect_args<TArgs ...>::collect(out, args ...);
  }
};
template<typename TLast>
struct collect_args<TLast> {
  static inline void collect(std::vector<NamedArgumentRef>& args, const TLast& x) {
    NamedArgumentRef arg = std::make_unique<NamedArgument>();
    arg->arg_name = "_" + std::to_string(args.size());
    arg->arg.name = arg->arg_name.c_str();
    get_arg_ti_arg<TLast>::get(arg->arg.argument, x);
    args.emplace_back(std::move(arg));
  }
};



template<typename TFunc>
struct Kernel {};

template<typename ... TArgs>
struct Kernel<std::function<void(TArgs ...)>> {
  std::function<void(TArgs ...)> fn_;

  Kernel(std::function<void(TArgs ...)> fn) : fn_(std::move(fn)) {}

  void compile(const TArgs& ... args) {
    std::vector<NamedArgumentRef> args2;
    args2.reserve(sizeof...(args));
    collect_args<TArgs ...>::collect(args2, args ...);
    std::cout << dump_taichi_aot_script(TI_ARCH_VULKAN, args2);
  }
};


template<typename TFunc>
struct get_func_ty {};
template<typename ... TArgs>
struct get_func_ty<void(TArgs ...)> {
  typedef std::function<void(TArgs ...)> type;
};
template<typename ... TArgs>
struct get_func_ty<void(*)(TArgs ...)> {
  typedef std::function<void(TArgs ...)> type;
};


template<typename TFunc>
auto to_kernel(TFunc f) {
  typename get_func_ty<TFunc>::type func { f };
  return Kernel<typename get_func_ty<TFunc>::type> { std::move(func) };
}

} // namespace ticpp

void kernel_impl(int i, float f, TiNdArray ndarray) {}

int main(int argc, const char** argv) {

  TiNdArray nd{};
  auto x = ticpp::to_kernel(kernel_impl);
  x.compile(1, 0.0, nd);

  //dump_taichi_aot_script(TI_ARCH_VULKAN, {});
  //system("python app.py");

  return 0;
}
