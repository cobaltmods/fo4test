# Native UI composition: ordinary overlays and HUDGlass menus

## Evidence and limits

Investigated with the OG Ghidra database and the supplied Shaders011.fxp.
The FXP explorer identifies this archive as the AE consumer layout (HUDGlass
section 101, rather than OG section 100). The extracted shaders are archived
under tools/captures/hudglass_analysis; extraction/decompilation does not modify
the installed game shaders. Runtime OG/AE pixel-for-pixel verification remains
an in-game test, not a result of static inspection.

- Scaleform FSolid writes RGBA. Ordinary Scaleform blend states use RGBA write
  masks and source-over alpha, while mode 15 preserves destination alpha.
  There is no evidence that every opaque Scaleform primitive selects mode 15.
- StartMenuBase::SetMenuColor (OG 140BBD240) selects player RGB separately from
  background settings. BSGFxShaderFXTarget::GetBackgroundQuads (1420F2720)
  transfers GetAlphaNormalized into the quad's alpha independently of RGB.
- FlatScreenModel::InitModel (140A01FB0) selects HUDGlass, HUDGlassFlat and
  ModMenuShadow materials. This is not just an ordinary Scaleform overlay.
- ScopeMenu constructors (OG 140BB6DF0, AE 140AFC570) create a dedicated
  renderer, attach a HUDGlassFlat model, enable post-AA rendering (offset 0x53),
  and select `Materials\Interface\HUDGlassFlat.BGEM`. The material string was
  read through the actual pointer in each database. Scope is not named
  FlatScreenModel and therefore needs material-based recognition as well.
- Interface3D::RenderAll (140AE7FB0) calls RenderPrepassesAndMenus, then
  RenderMain for each enabled renderer. Resolve must precede those draws.
- Bethesda CreateAlphaBlendStateObjects (141D4FE40) is distinct from Scaleform:
  its straight-alpha state uses SRC_ALPHA / INV_SRC_ALPHA for both RGB and
  alpha; its multiply state uses DEST_COLOR / ZERO for RGB. Write mode 1 is
  RGB-only, and write mode 2 is RGBA. These facts do not identify the state of
  an individual live draw by themselves.
- BSImagespaceShader::SetupTechnique (1428C83D0) takes its write mode from
  1438CB3E8, whose static default is 2 (RGBA). Do not claim HUDGlassCopy always
  drops alpha simply because other engine draws can use RGB-only writes.

## Shader equations

HUDGlass, with A = t0 and B = t1:

```
RGB = A.rgb * A.a + 2 * B.rgb * (1 - A.a)
alpha = A.a * A.a + 0.5 * saturate(0.9 * B.a) * (1 - A.a)
```

HUDGlassDropShadow, with S the sum of five neighboring samples:

```
RGB = center.rgb + 0.02 * S.rgb
alpha = vertexAlpha * saturate(center.a + 2.5 * S.a)
```

HUDGlassCopy multiplies sampled RGBA by cb2[0]; it does not premultiply RGB by
the resulting alpha. BlurX/Y average all four channels. Clear writes zero.
These intermediate values are governed by the engine's subsequent blending,
not a universal coverage-alpha interpretation at our final Present.

The screenshots use a black world. If the actual composite input RGB is zero,
our final source-over expression produces UI.rgb irrespective of UI.a. Thus
the visible brightness difference alone cannot prove final alpha loss, and
must not be described as brighter world leaking through.

## Selected design

Keep ordinary UI on the existing split path with premultiplied source-over.
Do not infer alpha from RGB brightness and do not globally modify engine blend
states, shader alpha, or stencil/mask behavior.

For enabled renderers with HUDGlass/HUDGlassWithMod, or a screen-attached model
using HUDGlassFlat.BGEM, during the post-AA RT0 pass, resolve the current world
and preceding overlay into the UI target BEFORE RenderPrepassesAndMenus.
The trigger is an actual D3D12 present override, not ENB or FG being enabled.
ENB uses its native UI allocation; non-ENB uses its native proxy buffer.
Only common render/model call hooks are installed for non-ENB; ENB allocation,
depth and viewport routing are not enabled there. Then run both engine
calls unchanged against an ordinary complete-frame destination. Preserve their
final RGB rather than trying to reconstruct all of their intermediate alpha
semantics. This is a compatibility path, not an assertion that a particular
unobserved live draw selected multiply or RGB-only blending.

Reuse the existing menu resolve resource and D3D11/D3D12 GPU fence chain.
Preserve OM bindings across the resolve. nativeUIIncludesScene deduplicates
ScopeMenu/framework/engine resolves and bypasses the final world+UI composite
in Present and screenshots. No engine window resize, shader replacement,
Scaleform replay, new detour, or unconditional gameplay round trip is added.
The cost is one existing full-frame interop resolve on affected menu frames.
Other materials/effects are not assumed compatible. ScopeMenu's previous
ENB BeginNativeUI resolve and non-ENB upscaler copyback exceptions, including
the menu-open state, are removed: its material now selects the common resolve.
This avoids interpreting the completed black surround as an alpha overlay;
it does not reconstruct alpha from RGB or fix arbitrary zero-alpha draws.

## Validation

Build and source/diff checks are local checks only. In-game acceptance requires:

1. White and black UI colors: PauseMenu background, text, and highlight match
   the mod-OFF reference with the same scene and menu state.
2. Confirm the one-time `[UI composite] HUDGlass uses engine frame composition`
   log is present (the first qualifying renderer emits it).
3. Open/close PauseMenu and return to gameplay; the complete-frame flag must
   reset per frame and ordinary split rendering resume.
4. ScopeMenu, framework backgrounds, and same-frame screenshots must not
   receive a second scene composite.
5. Check quality changes, ENB/non-ENB, and FG on/off. Test ScopeMenu separately
   with its black surround, especially on non-ENB DLSS/FSR present overrides.
   Static constructor/material evidence supports the common path, but does not
   substitute for an in-game test of installed menu/model replacements.
