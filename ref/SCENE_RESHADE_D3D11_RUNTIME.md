# ENB D3D11 ReShade runtime

Implemented as a D3D11 scene-runtime experiment (2026-09-10). Normal ReleaseDbg build,
nr_capture=false, no test harness or unit tests. Game behavior is unverified.

## API/source verification

- ReShade's public `create_effect_runtime` accepts a D3D11 device, immediate
  context and an existing IDXGISwapChain. Its D3D11 implementation in
  https://github.com/crosire/reshade/blob/main/source/addon.cpp checks that
  GetDevice returns the supplied device, then constructs D3D11 API wrappers.
- https://github.com/crosire/reshade/blob/main/source/d3d11/d3d11_impl_swapchain.cpp
  obtains buffer 0 and the window from GetDesc. It does not require a real
  flip-model swapchain for this path.
- `update_and_present_effect_runtime` updates the manually owned runtime and
  flushes its command queue. It does not call DXGI Present. This supplies effect
  loading, frame uniforms, input updates and per-frame render-guard reset.

## Implemented flow

ENB or non-ENB late imagespace -> D3D11 active render-rectangle copy -> manual
ReShade runtime -> copy result back -> existing SR/NR input capture -> SR/NR ->
native game UI -> real Present.

There is no setting or menu section. ReShade add-on support being available
activates the scene runtime during upscaling. SR disabled retains normal output
ReShade behavior. `SceneReShade` replaces the earlier ENB-only prototype name.

SceneSwapChain presents one persistent D3D11 texture at the active render size,
with the source's device and game HWND. Both color copies cover only that active
rectangle. Depth uses a D3D11 Load-only compute pass into matching R32_FLOAT
storage; no spatial jitter correction is performed. Color and depth retain the
same raster coordinates for SR. No D3D12 depth sharing or fences are added.

Runtime creation uses the existing ReShade.ini beside Fallout4.exe directly.
No plugin-specific configuration copy is created. Existing presets remain in use.
The real output runtime's automatic effects and overlay are suppressed while
this manually owned scene runtime exists. The scene overlay is itself upscaled.

Resize tears down the scene runtime before replacing engine targets. Render-size,
format and device changes also recreate its resources. Disabling SR resets it.
Initialization failure leaves the normal output runtime available and is logged;
it is not retried every frame. The next game launch can retry initialization.

## Validation and limits

ReleaseDbg build and source review; no unit tests or harness. Game behavior is
unverified for ENB and non-ENB, OG and AE. No new game relocations are introduced.
Added frame work: two render-size D3D11 color copies and one depth compute pass.
Both runtimes may retain effect resources. Effects can reload on render-size
changes. Frozen/loading paths without world upscaling do not update the scene
runtime. Arbitrary shaders/add-ons and low-resolution overlay input still need
runtime validation. There is no claim of temporally stable depth silhouettes.
