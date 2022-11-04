#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <initializer_list>
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








struct Expr {
  virtual ~Expr() {}
  virtual void to_string(std::stringstream& ss) const = 0;

  template<typename T>
  static std::shared_ptr<Expr> create(T&& x) {
    return std::shared_ptr<Expr>(static_cast<Expr*>(new T(std::move(x))));
  }
};
typedef std::shared_ptr<Expr> ExprRef;

struct Stmt {
  virtual ~Stmt() {}
  virtual void to_string(std::stringstream& ss) const = 0;

  template<typename T>
  static std::shared_ptr<Stmt> create(T&& x) {
    return std::shared_ptr<Stmt>(static_cast<Stmt*>(new T(std::move(x))));
  }

  void commit() const;
};
typedef std::shared_ptr<Stmt> StmtRef;





struct ScopeContext {
  size_t indent = 4;
  std::stringstream ss;

  void commit_stmt(const Stmt* stmt) {
    ss << std::string(indent, ' ');
    stmt->to_string(ss);
    ss << std::endl;
  }

  std::string finish() {
    std::string out = ss.str();
    ss.clear();
    return out;
  }
};
thread_local ScopeContext scope_context_;


void Stmt::commit() const {
  scope_context_.commit_stmt(this);
}






struct AddExpr : public Expr {
  ExprRef a_;
  ExprRef b_;

  static ExprRef create(const ExprRef& a, const ExprRef& b) {
    AddExpr out {};
    out.a_ = a;
    out.b_ = b;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    ss << "(";
    a_->to_string(ss);
    ss << "+";
    b_->to_string(ss);
    ss << ")";
  }
};
struct IntImmExpr : public Expr {
  std::string arg_name_;
  int32_t value_;

  static ExprRef create(const std::string& arg_name) {
    IntImmExpr out {};
    out.arg_name_ = arg_name;
    return Expr::create(std::move(out));
  }
  static ExprRef create(int32_t value) {
    IntImmExpr out {};
    out.value_ = value;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    if (arg_name_.empty()) {
      ss << "ti.i32(" << value_ << ")";
    } else {
      ss << arg_name_;
    }
  }

  friend ExprRef operator+(const ExprRef& a, const ExprRef& b) {
    return AddExpr::create(a, b);
  }
};
struct FloatImmExpr : public Expr {
  std::string arg_name_;
  float value_;

  static ExprRef create(const std::string& arg_name) {
    FloatImmExpr out {};
    out.arg_name_ = arg_name;
    return Expr::create(std::move(out));
  }
  static ExprRef create(int32_t value) {
    FloatImmExpr out {};
    out.value_ = value;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    if (arg_name_.empty()) {
      ss << "ti.f32(" << value_ << ")";
    } else {
      ss << arg_name_;
    }
  }
};
struct IterVarExpr : public Expr {
  std::string name_;
  size_t begin_;
  size_t end_;
  size_t step_;

  static ExprRef create(size_t begin, size_t end, size_t step) {
    static size_t id_counter_ = 0;
    IterVarExpr out {};
    out.name_ = "it_" + std::to_string(id_counter_++);
    out.begin_ = begin;
    out.end_ = end;
    out.step_ = step;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    ss << name_;
  }
};
struct IndexExpr : public Expr {
  ExprRef alloc_;
  ExprRef index_;

  static ExprRef create(const ExprRef& alloc, const ExprRef& index) {
    IndexExpr out {};
    out.alloc_ = alloc;
    out.index_ = index;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    alloc_->to_string(ss);
    ss << "[";
    index_->to_string(ss);
    ss << "]";
  }
};
struct TupleExpr : public Expr {
  std::vector<ExprRef> elems_;

  static ExprRef create(std::vector<ExprRef>&& elems) {
    TupleExpr out {};
    out.elems_ = elems;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    if (elems_.empty()) {
      ss << ",";
    } else {
      for (const auto& elem : elems_) {
        ss << "(";
        elem->to_string(ss);
        ss << "),";
      }
    }
  }
};
struct NdArrayAllocExpr : public Expr {
  std::string arg_name_;
  TiNdArray ndarray_;

  static ExprRef create(const TiNdArray& ndarray) {
    static size_t id_counter_ = 0;
    NdArrayAllocExpr out {};
    out.arg_name_ = "ndarray_" + std::to_string(id_counter_);
    out.ndarray_ = ndarray;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    ss << arg_name_;
  }
};





struct StoreStmt : public Stmt {
  ExprRef dst_;
  ExprRef value_;

  static StmtRef create(const ExprRef& dst, const ExprRef& value) {
    StoreStmt out {};
    out.dst_ = dst;
    out.value_ = value;
    return Stmt::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    dst_->to_string(ss);
    ss << "=";
    value_->to_string(ss);
  }
};







template<typename T>
struct get_arg_ti_arg {};
template<>
struct get_arg_ti_arg<int32_t> {
  static inline void get(TiArgument& arg, int32_t value) {
    arg.type = TI_ARGUMENT_TYPE_I32;
    arg.value.i32 = value;
  }
  static inline ExprRef to_expr(int32_t value) {
    return IntImmExpr::create(value);
  }
};
template<>
struct get_arg_ti_arg<float> {
  static inline void get(TiArgument& arg, float value) {
    arg.type = TI_ARGUMENT_TYPE_F32;
    arg.value.f32 = value;
  }
  static inline ExprRef to_expr(float value) {
    return FloatImmExpr::create(value);
  }
};
template<>
struct get_arg_ti_arg<TiNdArray> {
  static inline void get(TiArgument& arg, const TiNdArray& value) {
    arg.type = TI_ARGUMENT_TYPE_NDARRAY;
    arg.value.ndarray = value;
  }
  static inline ExprRef to_expr(const TiNdArray& value) {
    return NdArrayAllocExpr::create(value);
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





std::string dump_taichi_aot_script(TiArch arch, const std::vector<NamedArgumentRef>& args, const std::string& code) {
  std::stringstream ss;
  ss << R"(
import taichi as ti

ti.init()" << arch2str(arch) << R"()

)" << build_symbols(args) << R"(

@ti.kernel
def f()" << build_params(args) << R"():
)" << code << R"(

g_builder = ti.graph.GraphBuilder()
g_builder.dispatch(f,)" << build_args(args) << R"()
graph = g_builder.compile()

mod = ti.aot.Module()" << arch2str(arch) << R"()
mod.add_graph('g', graph)
mod.save("module", '')
)";
  return ss.str();
}




template<typename TFunc>
struct Kernel {};

template<typename ... TExprs>
struct Kernel<std::function<void(TExprs ...)>> {
  std::function<void(TExprs ...)> fn_;

  Kernel(std::function<void(TExprs ...)> fn) : fn_(std::move(fn)) {}

  template<typename ... TArgs>
  void compile(const TArgs& ... args) {
    static_assert(sizeof...(TArgs) == sizeof...(TExprs), "");

    std::vector<NamedArgumentRef> args2;
    args2.reserve(sizeof...(args));
    collect_args<TArgs ...>::collect(args2, args ...);
    fn_(get_arg_ti_arg<TArgs>::to_expr(args) ...);
    std::cout << dump_taichi_aot_script(TI_ARCH_VULKAN, args2, scope_context_.finish());
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

void kernel_impl(ticpp::ExprRef i, ticpp::ExprRef f, ticpp::ExprRef ndarray) {
  auto stmt = ticpp::StoreStmt::create(
    ticpp::IndexExpr::create(ndarray, ticpp::TupleExpr::create({ i, i })), f);
  stmt->commit();
}

int main(int argc, const char** argv) {

  TiNdArray nd{};
  auto x = ticpp::to_kernel(kernel_impl);
  x.compile(1, 1.23f, nd);

  return 0;
}
