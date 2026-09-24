 # Butter

A general-purpose Vulkan renderer for Linux, speaking to both Wayland and X11
(XCB). Part of the `htils` `bread` `butter` `cheese` stack; `cheese`
(immediate-mode UI) is its first consumer.

## Current State

Usable, in active development. 2D rendering is solid and the post-processing
path (targets, explicit passes, effects) is in place; 3D is not implemented yet.
The API may still change.

## Backends

Chosen at build time:

- `-DBUTTER_WAYLAND` - Wayland
- `-DBUTTER_X11` - X11 (XCB)

## Features

- **Vulkan renderer** - backend-agnostic core (Wayland / X11).
- **Threaded or manual rendering** - hand it a draw callback and it runs a render
  thread (`butter_start_render_thread`, `butter_request_frame`,
  `butter_wait_for_frame` / `butter_frame_completed`), or drive frames yourself
  with `butter_begin_frame` / `butter_end_frame`.
- **Batched drawing API** - `butter_submit_draws` takes an array of
  `butter_draw_cmd_t`; pipelines, vertex/index buffers, scissors and textures are
  bound with minimal redundant state changes. Dynamic vertex/index buffers via
  `butter_alloc_vertices` / `butter_alloc_indices`.
- **Pipelines** - `butter_create_pipeline` from a `butter_pipeline_desc_t`
  (shaders, attributes, topology, blend, depth, push constants), returned as an
  opaque `butter_pipeline_t *`. Butter owns and tracks them, and rebuilds them
  in place when render resources change.
- **Render targets** - `butter_target_create` makes an offscreen, sampleable
  target. A target is just a texture (`butter_target_texture`), so
  render-to-texture, magnification and snapshots fall out of the same type.
  Targets are live: butter rebuilds them in place on resize and AA changes.
- **Explicit passes** - `butter_pass_begin` / `butter_pass_end` open a pass on a
  target (or the swapchain when `null`); a pass can be broken and resumed
  (`color_load`) so work stays in draw order. A frame is a pass on the swapchain.
- **Post-processing** - `butter_submit_effect` runs a consumer fullscreen draw
  sampling one or more input textures, with push constants; `butter_snapshot`
  copies the frame so far into a target. Filters and composites are just effects
  chained over targets - blur is a thin layer built on this, not core code.
  Blend modes cover alpha, additive, premultiplied, multiply, screen, min, max.
- **Shaders** - SPIR-V, loaded from file (`butter_shader_load_file`), memory
  (`butter_shader_from_memory`), or looked up from a name-deduplicated registry
  (`butter_shader_get`).
- **Textures** - registry (`butter_texture_register` / `_deregister` / `_get` /
  `_is_ready`), uploaded on a background thread or synchronously, with partial
  region updates (`butter_update_texture_region`).
- **Anti-aliasing** - MSAA with device-support discovery (`butter_get_aa_caps`);
  switch mode and sample count at runtime (`butter_set_aa_mode` /
  `butter_set_aa_samples`) and the render pass and pipelines are rebuilt for you.
  Works across pass break/resume, so effects compose with MSAA.
- **Frame pacing** - vsync (`butter_set_vsync`), target refresh rate
  (`butter_set_target_refresh_rate`), and a configurable latency cap.
- **Stats** - `butter_get_stats` reports CPU/GPU frame times, GPU usage, frame
  rate, and VRAM usage/budget (`VK_EXT_memory_budget`).

## Building

Butter builds with [conjure](https://codeberg.org/h4rl/conjure):

```
conjure build -p wayland-release          # libbutter-wayland.so
conjure build -p wayland-release-static   # libbutter-wayland-static.a
conjure build -p wayland-debug            # libbutter-wayland-debug.a
# ...and the x11-* equivalents
```

Profiles are `<wayland|x11>-<debug|release|release-static>`. Debug builds are
static archives; release builds are shared unless built with the `-static`
profile. Each library profile also emits a pkg-config file under
`lib/<profile>/pkgconfig/`.

A Nix flake provides the six library packages plus a dev shell:

```
nix build .#butter-wayland-release
nix develop
```

## Tests

`src/test` holds small windowed examples - a triangle, and a textured rounded fan
that cycles MSAA with **A** (the fan's arcs make the AA effect obvious). The
textured example also exercises the post-processing path: it renders into a
target, submits an effect, and composites a separable Gaussian backdrop blur
built entirely from the public API (its GLSL lives in `src/test`). Build them
with `conjure test -p wayland-debug` (or `x11-debug`).

## License

This project is licensed under the BSD-3 Clause License - See the
[LICENSE](LICENSE) file for details.