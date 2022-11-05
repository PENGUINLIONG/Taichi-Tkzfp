
import taichi as ti

ti.init(ti.vulkan, offline_cache=False)

sym_0 = ti.graph.Arg(ti.graph.ArgKind.SCALAR, '_0', ti.i32)
sym_1 = ti.graph.Arg(ti.graph.ArgKind.SCALAR, '_1', ti.f32)
sym_2 = ti.graph.Arg(ti.graph.ArgKind.NDARRAY, '_2', ti.f32, field_dim=2, element_shape=(2, ))


@ti.kernel
def f(_0: ti.i32, _1: ti.f32, _2: ti.types.ndarray(field_dim=2), ):
    _2[(ti.i32(0)),(_0),].fill(_1)


g_builder = ti.graph.GraphBuilder()
g_builder.dispatch(f,sym_0, sym_1, sym_2, )
graph = g_builder.compile()

mod = ti.aot.Module(ti.vulkan)
mod.add_graph('g', graph)
mod.save("module", '')

