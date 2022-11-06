// Code generator.
// @PENGUINLIONG
#pragma once
#include "ticpp/parse_context.hpp"

namespace ticpp {

extern std::string composite_python_script(
  TiArch arch,
  const ParseResult& itm
);



template<typename TFunc, typename ... TArgs>
std::string run_codegen(TiArch arch, TFunc& fn, TArgs ... args) {
  PARSE_CONTEXT.start();
  fn(expr_conv_t<TArgs>::to_expr(args) ...);
  ParseResult itm = PARSE_CONTEXT.stop();

  std::string out = composite_python_script(arch, itm);
  return out;
}



template<typename ... TArgs>
struct assign_cgraph_args_t {};
template<typename TFirst>
struct assign_cgraph_args_t<TFirst> {
  static void assign(ti::ComputeGraph& cgraph, uint32_t counter, const int32_t& x) {
    cgraph["_" + std::to_string(counter)] = x;
  }
  static void assign(ti::ComputeGraph& cgraph, uint32_t counter, const float& x) {
    cgraph["_" + std::to_string(counter)] = x;
  }
  static void assign(ti::ComputeGraph& cgraph, uint32_t counter, const TiNdArray& x) {
    cgraph["_" + std::to_string(counter)] = x;
  }
  template<typename U>
  static void assign(ti::ComputeGraph& cgraph, uint32_t counter, const ti::NdArray<U>& x) {
    cgraph["_" + std::to_string(counter)] = x.ndarray();
  }
};
template<typename TFirst, typename ... TArgs>
struct assign_cgraph_args_t<TFirst, TArgs ...> {
  static void assign(ti::ComputeGraph& cgraph, uint32_t counter, const TFirst& x, const TArgs& ... args) {
    assign_cgraph_args_t<TFirst>::assign(cgraph, counter, x);
    assign_cgraph_args_t<TArgs ...>::assign(cgraph, counter + 1, args ...);
  }
};



extern void dump_aot_script(const std::string& script);
extern std::string load_aot_module_path();
extern void remove_aot_artifact(const std::string& temp_dir);

template<typename TFunc>
struct Kernel {};
template<typename ... TValues>
struct Kernel<std::function<void(TValues ...)>> {
  ti::Runtime runtime_;
  std::function<void(TValues ...)> fn_;

  // Ready after instantiation.
  ti::AotModule mod_;
  ti::ComputeGraph cgraph_;

  Kernel(const ti::Runtime& runtime, std::function<void(TValues ...)> fn) :
    runtime_(runtime.arch(), runtime.runtime(), false), fn_(std::move(fn)) {}

  template<typename ... TArgs>
  void instantiate(const TArgs& ... args) {
    if (mod_.is_valid()) { return; }

    static_assert(sizeof...(TArgs) == sizeof...(TValues), "");

    // Run codegen.
    std::string script = run_codegen(runtime_.arch(), fn_, args ...);

    // Dump and run the AOT script.
    dump_aot_script(script);

    std::string path = load_aot_module_path();
    std::cout << path << std::endl;

    // Load compute graph.
    mod_ = runtime_.load_aot_module(path);
    cgraph_ = mod_.get_compute_graph("g");

    remove_aot_artifact(path);
  }

  template<typename ... TArgs>
  void launch(const TArgs& ... args) {
    instantiate(args ...);

    assign_cgraph_args_t<TArgs ...>::assign(cgraph_, 0, args ...);
    cgraph_.launch();
  }

  template<typename ... TArgs>
  void operator()(const TArgs& ... args) {
    launch(args ...);
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
auto to_kernel(ti::Runtime& runtime, TFunc f) {
  typename get_func_ty<TFunc>::type func { f };
  return Kernel<typename get_func_ty<TFunc>::type>(runtime, std::move(func));
}



} // namespace ticpp
