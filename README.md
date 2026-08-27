# Glasscope

Glasscope is a liquid-glass magnifier that follows the pointer on Hyprland. It
is useful for reading small text, checking pixels, or taking a closer look at a
part of the desktop without changing the application underneath.

## Features

- A smooth magnifying lens with adjustable zoom, size, refraction, colour
  dispersion and edge shape.
- A soft trailing shape while the pointer moves, followed by a short liquid
  bounce when it stops.
- Smooth filtering for everyday use and nearest-neighbour sampling for pixel
  inspection.
- Pointer input passes through normally, the lens does not capture clicks or
  block the application below it.
- Lua functions and Hyprland dispatchers for showing, hiding and adjusting the
  lens from custom bindings.

## Before installing

Glasscope magnifies the image Hyprland has already composed. It cannot ask an
application to redraw text or images at a higher resolution, so a larger zoom
does not create detail that was not present in the original frame.

The plugin currently requires Hyprland's OpenGL renderer and an unrotated
output. It stays inactive on the lock screen. Because Hyprland plugins are tied
to the version they were built against, rebuild or update Glasscope after a
Hyprland upgrade.

## Requirements

Arch Linux dependencies are Hyprland development headers, CMake, a C++23
compiler, pkg-config, GLESv2 and Lua.

## Install with HyprPM

```bash
REPOSITORY_URL=https://github.com/horizon0427/glasscope
hyprpm update
hyprpm add "$REPOSITORY_URL"
hyprpm enable glasscope
hyprpm reload -n
```

Check that the plugin loaded successfully:

```bash
hyprpm list
hyprctl plugin list
hyprctl configerrors
```

Loading the plugin and turning on the lens are separate steps. Glasscope starts
with its own `enabled` option set to `false`; the configuration below enables
it and adds a toggle binding.

## Configuration

Glasscope settings live under `plugin.glasscope`. Add the following block to
your Hyprland Lua configuration. It uses a compact `130` px lens; the built-in
radius is `190` if no value is supplied.

```lua
if os.getenv("HYPR_NO_PLUGINS") ~= "1" and hl.plugin.glasscope ~= nil then
    hl.config({
        plugin = {
            glasscope = {
                enabled = true,

                radius = 130,          -- Geometry
                edge_width = 22.0,

                zoom = 2.0,            -- Optics
                bulge = 0.08,
                refraction = 1.0,
                dispersion = 0.7,
                edge_strength = 1.25,

                motion_strength = 1.5, -- Motion
                nearest = false,       -- Sampling
            },
        },
    })

    hl.bind("SUPER + CTRL + G", function()
        hl.plugin.glasscope.toggle()
    end, { description = "Toggle Glasscope liquid lens" })
end
```

Pixel dimensions are logical pixels, so the physical size follows the scale of
the monitor.

### Public options

| Group | Option | Type | Default | Range | Meaning |
| --- | --- | --- | --- | --- | --- |
| Runtime | `enabled` | bool | `false` | bool | Master switch for rendering and interaction. Starts fail-closed. |
| Geometry | `radius` | int | `190` | `80..600` | Resting lens radius. Larger lenses use more GPU time. |
| Geometry | `edge_width` | float | `22.0` | `4..48` | Width of the optical rim. |
| Optics | `zoom` | float | `2.0` | `1..6` | Base magnification. At `1`, `bulge` may still add a small center warp. |
| Optics | `bulge` | float | `0.08` | `0..0.28` | Additional convex magnification near the center. |
| Optics | `refraction` | float | `1.0` | `0..2` | Strength of edge displacement and optical tint. |
| Optics | `dispersion` | float | `0.7` | `0..2` | Amount of red/blue separation around the rim. |
| Optics | `edge_strength` | float | `1.25` | `0..2.5` | Master rim intensity for refraction, tint, shade, highlight and dispersion. |
| Motion | `motion_strength` | float | `1.5` | `0..2.5` | Strength of the trail, moving ripples and stop bounce. `0` keeps a circle. |
| Sampling | `nearest` | bool | `false` | bool | Nearest-neighbor sampling for pixel inspection. Normal reading usually looks better with `false`. |

### Controls

Glasscope adds the following Lua functions:

| Function | What it does |
| --- | --- |
| `toggle()` | Show or hide the lens at the pointer. |
| `show()` | Show the lens. |
| `hide()` | Hide the lens without disabling the plugin. |
| `adjust_zoom(delta)` | Change the current zoom. |
| `adjust_radius(delta)` | Change the current radius. |
| `adjust_edge_width(delta)` | Change the current edge width. |

The three adjustments are temporary and reset on the next Hyprland config
reload. They can be used in any `hl.bind` callback, for example:

```lua
hl.bind("SUPER + ALT + mouse_up", function()
    hl.plugin.glasscope.adjust_zoom(0.2)
end)
```

The equivalent Hyprland dispatchers are `glasscope:toggle`, `glasscope:show`
and `glasscope:hide`.

### Hidden, disabled and unloaded

`hide()` only hides the lens. Setting `enabled = false` stops Glasscope
completely and releases its GPU resources. Re-enabling it does not show the lens
until `show()` or `toggle()` is called.

HyprPM's enabled state is separate: `hyprpm disable glasscope` unloads the shared
object, while `plugin.glasscope.enabled = false` keeps the Lua and dispatcher
namespaces available.

## Uninstall

Remove the plugin through HyprPM:

```bash
hyprpm disable glasscope
hyprpm remove glasscope
hyprpm reload -n
```

Remove the `plugin.glasscope` block and any Glasscope bindings from your
Hyprland configuration, then reload it.

## Troubleshooting

- If Glasscope stops loading after a Hyprland update, run `hyprpm update` to
  rebuild it against the new version.
- Make sure `plugin.glasscope.enabled` is `true`; this is separate from
  `hyprpm enable glasscope`.
- Glasscope currently works only with the OpenGL renderer and an unrotated
  output.
- Run `hyprctl configerrors` and `hyprctl plugin list` when checking a failed
  configuration or load.
- `HYPR_NO_PLUGINS=1` can be used as a session-wide escape hatch if your own
  startup configuration supports it.

## Build from source

```bash
make
```

Or use CMake directly:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
```
