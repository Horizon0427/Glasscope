# Glasscope

A liquid-glass magnifier and RGB colour picker for Hyprland. Follow the pointer
or pin a live lens in place to inspect details on your desktop.

https://github.com/user-attachments/assets/f55f0095-4e23-4923-bee5-b4598ca9c9a0

- Adjustable magnification and lens size.
- Liquid motion, click feedback and elastic surface dragging.
- Colour picking with a crosshair, live swatch and clipboard output.
- Custom glass colours and smooth or nearest-neighbour sampling.

## Requirements

Developed and tested with **Hyprland 0.56.2 and Lua configuration**.

- Hyprland development headers matching the running compositor.
- CMake 3.25+, a C++23 compiler, pkg-config, GLESv2 and Lua development files.
- Hyprland's OpenGL renderer and an unrotated output.
- `wl-clipboard` for copying sampled colours.

Rebuild Glasscope after upgrading Hyprland.

## Installation

Choose one installation method.

### Build and load directly

```bash
git clone https://github.com/Horizon0427/glasscope.git
cd glasscope
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
install -Dm755 build/glasscope.so \
  "$HOME/.local/lib/hyprland-plugins/glasscope/glasscope.so"
```

Add this loader to your Hyprland Lua configuration, before the Glasscope
configuration below:

```lua
if os.getenv("HYPR_NO_PLUGINS") ~= "1" then
    hl.plugin.load(
        os.getenv("HOME") .. "/.local/lib/hyprland-plugins/glasscope/glasscope.so"
    )
end
```

### Install with HyprPM

```bash
hyprpm update
hyprpm add https://github.com/Horizon0427/glasscope
hyprpm enable glasscope
hyprpm reload -n
```

For HyprPM startup loading, add:

```lua
hl.on("hyprland.start", function()
    if os.getenv("HYPR_NO_PLUGINS") ~= "1" then
        hl.exec_cmd("hyprpm reload -n")
    end
end)
```

## Quick start

Add this after the plugin loader. It enables a 130 px lens at 2× magnification,
with a subtle colour palette and four shortcuts.

```lua
if os.getenv("HYPR_NO_PLUGINS") ~= "1" and hl.plugin.glasscope ~= nil then
    hl.config({
        plugin = {
            glasscope = {
                enabled = true,
                radius = 130,
                zoom = 2.0,
                color_strength = 0.3,
                color_width = 18.0,
                colors = {
                    transmission = "rgba(ffb8740d)",
                    refraction = "rgba(afd428a6)",
                    reflection = "rgba(f77a9599)",
                    highlight = "rgba(efe0d580)",
                },
            },
        },
    })

    hl.bind("SUPER + SHIFT + A", function()
        hl.plugin.glasscope.toggle()
    end, { description = "Show or hide Glasscope" })

    hl.bind("SUPER + ALT + A", function()
        hl.plugin.glasscope.toggle_pin()
    end, { description = "Pin or unpin Glasscope" })

    hl.bind("SUPER + ALT + C", function()
        hl.plugin.glasscope.begin_color_probe()
    end, { description = "Pick a colour" })

    hl.bind("SUPER + ALT + Escape", function()
        hl.plugin.glasscope.cancel_color_probe()
    end, { description = "Cancel colour picking" })
end
```

Apply the configuration:

```bash
hyprctl reload config-only
```

## Using Glasscope

With the example configuration:

| Shortcut | Action |
| --- | --- |
| `Super+Shift+A` | Show or hide the lens. |
| `Super+Alt+A` | Pin the lens or resume following the pointer. |
| `Super+Alt+C` | Start colour picking; left-click to confirm. |
| `Super+Alt+Escape` | Cancel colour picking. |

Open the lens before pinning, picking colours or adjusting its parameters.

**Click and drag:** while pinned, click the lens to make it bounce, or drag its
surface and release to let it spring back. The lens stays anchored and the view
remains live.

**Pick a colour:** aim the crosshair and left-click to copy `rgb(R, G, B)` to the
clipboard. Picking temporarily follows the pointer, then restores the previous
pin after confirmation or cancellation. The lens stays open until you hide it.

## Configuration

Add settings inside the `glasscope` table in the quick-start example. Dimensions
are logical pixels and follow the monitor scale. The table lists built-in defaults.

| Option | Default | Range | Purpose |
| --- | --- | --- | --- |
| `enabled` | `false` | `true` / `false` | Enable Glasscope. |
| `radius` | `190` | `80..600` | Lens radius in whole pixels. |
| `zoom` | `2.0` | `1..6` | Magnification. |
| `nearest` | `false` | `true` / `false` | Nearest-neighbour sampling for pixel inspection. |
| `motion_strength` | `1.5` | `0..2.5` | Motion and deformation strength. |
| `interaction_bounce` | `1.0` | `0..2.5` | Pinned click and rebound deformation. |
| `bulge` | `0.08` | `0..0.28` | Extra magnification near the centre. |
| `refraction` | `1.0` | `0..2` | Edge refraction strength. |
| `dispersion` | `0.7` | `0..2` | Colour separation at the rim. |
| `edge_width` | `22.0` | `4..48` | Optical rim width. |
| `edge_strength` | `1.25` | `0..2.5` | Optical rim strength. |
| `color_strength` | `0.0` | `0..1` | Custom colour intensity. |
| `color_width` | `18.0` | `4..48` | Custom colour band width. |
| `colors.transmission` | `rgba(00000000)` | Hyprland colour | Body tint. |
| `colors.refraction` | `rgba(00000000)` | Hyprland colour | Inner rim tint. |
| `colors.reflection` | `rgba(00000000)` | Hyprland colour | Outer rim tint. |
| `colors.highlight` | `rgba(00000000)` | Hyprland colour | Highlight tint. |

Set `color_strength` above `0` to apply your palette. RGBA alpha controls each
colour's contribution. Increase `interaction_bounce` for stronger click and
rebound feedback.

<details>
<summary>Custom bindings: Lua API and dispatchers</summary>

Functions are available under `hl.plugin.glasscope` after loading the plugin.

| Function | Action |
| --- | --- |
| `toggle()` | Show or hide the lens. |
| `show()` | Show the lens. |
| `hide()` | Hide the lens and end active picking. |
| `toggle_pin()` | Switch between pinned and following modes. |
| `is_pinned()` | Read the current pin state. |
| `begin_color_probe()` | Start interactive colour picking. |
| `pick_color()` | Copy the current lens-centre colour. |
| `cancel_color_probe()` | End colour picking. |
| `adjust_zoom(delta)` | Adjust magnification. |
| `adjust_radius(delta)` | Adjust lens radius. |
| `adjust_edge_width(delta)` | Adjust optical rim width. |

Parameter adjustments apply while the lens is open and reset on configuration
reload. For example:

```lua
hl.bind("SUPER + ALT + mouse_up", function()
    hl.plugin.glasscope.adjust_zoom(0.2)
end)
```

Dispatcher names are `glasscope:toggle`, `glasscope:show`, `glasscope:hide`,
`glasscope:toggle-pin`, `glasscope:begin-color-probe`, `glasscope:pick-color` and
`glasscope:cancel-color-probe`. Use the Lua functions in Lua configuration.

</details>

## Troubleshooting

- **Lens does not appear:** check `hyprctl plugin list` and `hyprctl configerrors`,
  set `enabled = true`, then use the show/hide shortcut.
- **Version mismatch after an update:** rebuild against the current Hyprland headers.
- **Custom colours are faint:** increase `color_strength` and the palette's alpha values.
- **Colour is not copied:** install `wl-clipboard` and check that `wl-copy` is available.

## Uninstall

For direct loading, remove the `hl.plugin.load(...)` call, the Glasscope
configuration and its bindings, then reload Hyprland:

```bash
hyprctl reload config-only
rm "$HOME/.local/lib/hyprland-plugins/glasscope/glasscope.so"
```

For a HyprPM installation:

```bash
hyprpm disable glasscope
hyprpm remove glasscope
hyprpm reload -n
```

Then remove the Glasscope configuration and bindings from your Lua files.

## License

[BSD 3-Clause](LICENSE).
