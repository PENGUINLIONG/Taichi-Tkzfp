#include <fstream>
#include "ticpp/codegen.hpp"

namespace ticpp {

void dump_aot_script(const std::string& script) {
  {
    std::fstream f("app.py", std::ios::out | std::ios::trunc);
    f << script << std::endl;
  }
  system("TI_OFFLINE_CACHE=0 /Users/penguinliong/opt/anaconda3/bin/python3 app.py");
}
std::string load_aot_module_path() {
  std::string path;
  {
    std::fstream f("temp_dir", std::ios::in);
    f >> path;
  }
  return path;
}
void remove_aot_artifact(const std::string& temp_dir) {
  system("rm temp_dir");
  system("rm app.py");
  system(("rm -r " + temp_dir).c_str());
}

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
      ss << shape.dims[i] << ", ";
    }
  } else {
    ss << ",";
  }
  ss << ")";
  return ss.str();
}

std::string build_symbols(const std::vector<NamedArgumentRef>& args) {
  std::stringstream ss;
  for (const NamedArgumentRef& arg2 : args) {
    const TiNamedArgument& arg = arg2->arg;

    ss << "sym" << arg.name << " = ";

    switch (arg.argument.type) {
    case TI_ARGUMENT_TYPE_I32:
      ss << "ti.graph.Arg(ti.graph.ArgKind.SCALAR, '" << arg.name << "', ti.i32)";
      break;
    case TI_ARGUMENT_TYPE_F32:
      ss << "ti.graph.Arg(ti.graph.ArgKind.SCALAR, '" << arg.name << "', ti.f32)";
      break;
    case TI_ARGUMENT_TYPE_NDARRAY:
      ss << "ti.graph.Arg(ti.graph.ArgKind.NDARRAY, "
        << "'" << arg.name << "', "
        << dtype2str(arg.argument.value.ndarray.elem_type) << ", "
        << "field_dim=" << arg.argument.value.ndarray.shape.dim_count << ", "
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

    ss << arg.name << ": ";

    switch (arg.argument.type) {
    case TI_ARGUMENT_TYPE_I32:
      ss << "ti.i32";
      break;
    case TI_ARGUMENT_TYPE_F32:
      ss << "ti.f32";
      break;
    case TI_ARGUMENT_TYPE_NDARRAY:
      ss << "ti.types.ndarray(field_dim=" << arg.argument.value.ndarray.shape.dim_count << ")";
      break;
    default:
      assert(false);
    }
    ss << ", ";
  }
  return ss.str();
}

std::string build_args(const std::vector<NamedArgumentRef>& args) {
  std::stringstream ss;
  for (const NamedArgumentRef& arg : args) {
    ss << "sym" << arg->arg_name << ", ";
  }
  return ss.str();
}

std::string build_code(const std::vector<StmtRef>& stmts) {
  PythonScriptWriter ss;
  ss.push_indent();
  for (const StmtRef& stmt : stmts) {
    stmt->to_string(ss);
    ss.commit_line();
  }
  ss.pop_indent();
  return ss.str();
}

std::string composite_python_script(
  TiArch arch,
  const ParseResult& itm
) {
  std::stringstream ss;
  ss << R"(
import tempfile
import taichi as ti

ti.init()" << arch2str(arch) << R"(, offline_cache=False)

)" << build_symbols(itm.args) << R"(

@ti.kernel
def f()" << build_params(itm.args) << R"():
)" << build_code(itm.stmts) << R"(

g_builder = ti.graph.GraphBuilder()
g_builder.dispatch(f,)" << build_args(itm.args) << R"()
graph = g_builder.compile()

mod = ti.aot.Module()" << arch2str(arch) << R"()
mod.add_graph('g', graph)
temp_dir = tempfile.mkdtemp()
mod.save(temp_dir, '')
with open("temp_dir", "w") as f:
    f.write(temp_dir)
)";
  return ss.str();
}

} // namespace ticpp
