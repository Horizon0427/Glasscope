# Glasscope

A liquid-glass magnifier and colour picker for Hyprland. Follow the pointer,
inspect small details, and copy a pixel's RGB value without changing the
application underneath.

![Glasscope liquid-glass lens demonstration](assets/glasscope-demo.gif)

## Features

- A magnifying lens with refraction, colour dispersion and a convex centre.
- A soft trailing shape while moving, followed by a short liquid bounce when
  the pointer stops.
- Smooth filtering for reading and nearest-neighbour sampling for pixel work.
- Click-to-pick colour sampling with an adaptive crosshair, live swatch and
  `rgb(R, G, B)` clipboard output.
- Four configurable colour layers: transmission, inner refraction, outer
  reflection and highlight. Colour strength and band width are independent.
- Normal pointer input passes through. The left-button press and release used
  to confirm a colour pick are consumed so they do not click the application
  underneath.
- Lua functions and Hyprland dispatchers for custom bindings.

## Requirements

Developed and tested with **Hyprland 0.56.2 and its Lua configuration**.

- Hyprland development headers matching the running compositor.
- CMake 3.25 or newer, a C++23 compiler, pkg-config, GLESv2 and Lua development
  files.
- Hyprland's OpenGL renderer and an unrotated output.
- `wl-copy` from `wl-clipboard` for copying sampled colours.

Hyprland plugins are tied to the compositor version they were built against.
Rebuild Glasscope after a Hyprland upgrade. Glasscope stays inactive on the
lock screen.

## Installation

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

If your system provides `hyprpm`, you can use it to build and load the plugin:

```bash
hyprpm update
hyprpm add https://github.com/Horizon0427/glasscope
hyprpm enable glasscope
hyprpm reload -n
```

Use one loading method. The direct-loading Lua block is not needed when HyprPM
manages Glasscope. For HyprPM startup loading, add:

```lua
hl.on("hyprland.start", function()
    if os.getenv("HYPR_NO_PLUGINS") ~= "1" then
        hl.exec_cmd("hyprpm reload -n")
    end
end)
```

## Configuration and quick start

Add this block after the plugin loader. It enables a compact `130` px lens with
subtle lime and rose accents, and adds the example shortcuts listed below.
All colour values are written explicitly; no external palette generator is
required.

```lua
if os.getenv("HYPR_NO_PLUGINS") ~= "1" and hl.plugin.glasscope ~= nil then
    hl.config({
        plugin = {
            glasscope = {
                enabled = true,

                radius = 130,
                edge_width = 22.0,
                zoom = 2.0,
                bulge = 0.08,
                refraction = 1.0,
                dispersion = 0.7,
                edge_strength = 1.25,
                motion_strength = 1.5,
                nearest = false,

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
    end, { description = "Toggle Glasscope liquid lens" })

    hl.bind("SUPER + ALT + C", function()
        hl.plugin.glasscope.begin_color_probe()
    end, { description = "Start Glasscope colour picker" })

    hl.bind("SUPER + ALT + Escape", function()
        hl.plugin.glasscope.cancel_color_probe()
    end, { description = "Cancel Glasscope colour picker" })
end
```

Apply the configuration and check the plugin:

```bash
hyprctl reload config-only
hyprctl plugin list
hyprctl configerrors
```

| Example shortcut | Action |
| --- | --- |
| `Super+Shift+A` | Show or hide the magnifying lens. |
| `Super+Alt+C` | Start colour picking. |
| Left click while picking | Sample the pixel and copy its RGB value. |
| `Super+Alt+Escape` | Cancel picking without copying. |

These shortcuts come from the example configuration; Glasscope does not
install key bindings automatically. Loading the plugin and displaying the lens
are separate actions: use the toggle shortcut after loading it.

## Picking a colour

1. Press `Super+Alt+C` once, then release the keys. You do not need to hold the
   shortcut while picking.
2. Move the crosshair to the pixel you want. The swatch inside the lens follows
   the centre pixel's colour.
3. Left-click to confirm. Glasscope consumes that press and its matching release,
   restores normal cursor visibility, and copies text such as `rgb(175, 212, 40)`.
4. Paste the result into your editor or colour field. A short notification and
   swatch provide feedback. If the lens was hidden before picking, it hides
   again after that feedback fades.

Use `Super+Alt+Escape` to cancel without replacing the clipboard. With your own
bindings, call `begin_color_probe()` to start and `cancel_color_probe()` to
cancel. `pick_color()` also supports a one-shot pick at the lens centre without
first entering interactive probe mode.

Sampling happens **before Glasscope draws its magnification, refraction,
dispersion and custom colours**. Changing the lens colours therefore does not
change the sampled RGB value. The sample describes the composited desktop
pixel, which may differ from an asset's original colour because of application
or compositor colour processing.

The result currently uses `rgb(R, G, B)` text. `wl-clipboard` must be installed
for clipboard output.

## Configuring lens colours

### Strength and width

The two main controls are independent of the optical geometry:

- `color_strength`: `0.0` keeps the original liquid-glass appearance, including
  its built-in mint/lilac rim and specular highlight. Increase it toward `1.0`
  to add more of the configured colours. The example uses `0.3`.
- `color_width`: width of the custom colour band in logical pixels. Start with
  `18.0`; use `12.0` for a thinner band or `22.0` for a wider one. This does not
  change `edge_width`, which controls the refractive optical rim.

The built-in default for `color_strength` is `0.0`, so setting colour values
alone does not enable custom colouring. Set it above zero to see your palette.

### Four colour layers

| Option | Role | Explicit example |
| --- | --- | --- |
| `colors.transmission` | Gentle colour-channel absorption in the transmitted image. | `rgba(ffb8740d)` — warm amber, about 5% alpha. |
| `colors.refraction` | Colour on the inner part of the custom band. | `rgba(afd428a6)` — lime, about 65% alpha. |
| `colors.reflection` | Colour near its outer edge, weighted toward the light-facing side. | `rgba(f77a9599)` — rose, 60% alpha. |
| `colors.highlight` | Tint blended into the original specular highlight, retaining its shape and intensity coefficient. | `rgba(efe0d580)` — warm off-white, about 50% alpha. |

Use Hyprland's `rgba(RRGGBBAA)` format. The first six hex digits specify the
colour; the final two specify its individual contribution:

| Alpha suffix | Amount |
| --- | --- |
| `00` | Off |
| `40` | About 25% |
| `80` | About 50% |
| `ff` | Full |

For example, changing `rgba(afd428a6)` to `rgba(afd42880)` keeps the same lime
colour and reduces that layer's contribution. Alpha combines with
`color_strength` and the layer's spatial mask; it is not the opacity of the
whole lens.

For vivid colours, try saturated RGB values while keeping `color_strength`
moderate. Keep transmission subtle if you want the magnified content to stay
close to its original appearance. Reload with `hyprctl reload config-only`
after editing; colour changes do not require a rebuild and update even when
the lens is stationary.

### All options

Dimensions are logical pixels and follow the monitor scale.

| Option | Type | Built-in default | Range |
| --- | --- | --- | --- |
| `enabled` | bool | `false` | `true` / `false` |
| `radius` | int | `190` | `80..600` |
| `edge_width` | float | `22.0` | `4..48` |
| `zoom` | float | `2.0` | `1..6` |
| `bulge` | float | `0.08` | `0..0.28` |
| `refraction` | float | `1.0` | `0..2` |
| `dispersion` | float | `0.7` | `0..2` |
| `edge_strength` | float | `1.25` | `0..2.5` |
| `motion_strength` | float | `1.5` | `0..2.5` |
| `nearest` | bool | `false` | `true` / `false` |
| `color_strength` | float | `0.0` | `0..1` |
| `color_width` | float | `18.0` | `4..48` |
| `colors.transmission` | colour | `rgba(00000000)` | Hyprland colour |
| `colors.refraction` | colour | `rgba(00000000)` | Hyprland colour |
| `colors.reflection` | colour | `rgba(00000000)` | Hyprland colour |
| `colors.highlight` | colour | `rgba(00000000)` | Hyprland colour |

`bulge` controls additional convex magnification near the centre. `refraction`
controls edge displacement and optical tint; `dispersion` controls RGB
separation at the rim. `edge_strength` scales the rim effects, including shade
and highlight. `motion_strength = 0` keeps a circular shape. Enable `nearest`
for pixel inspection; leave it off for smoother reading.

## Lua API

Functions are available under `hl.plugin.glasscope` after the plugin loads.

| Function | Action |
| --- | --- |
| `toggle()` | Show or hide the lens at the pointer. |
| `show()` | Show the lens. |
| `hide()` | Hide the lens and cancel an active probe. |
| `begin_color_probe()` | Enter interactive picking until a left click or cancellation. |
| `pick_color()` | Request a centre-pixel sample and clipboard copy; also works as a one-shot action. |
| `cancel_color_probe()` | Leave probe mode without sampling. |
| `adjust_zoom(delta)` | Change the current zoom. |
| `adjust_radius(delta)` | Change the current radius. |
| `adjust_edge_width(delta)` | Change the current optical edge width. |

The three adjustments are temporary overrides and reset on the next
configuration reload. For example:

```lua
hl.bind("SUPER + ALT + mouse_up", function()
    hl.plugin.glasscope.adjust_zoom(0.2)
end)
```

The corresponding dispatcher names are `glasscope:toggle`, `glasscope:show`,
`glasscope:hide`, `glasscope:begin-color-probe`, `glasscope:pick-color` and
`glasscope:cancel-color-probe`. In Lua configuration, use the functions above.

### Hidden, disabled and unloaded

`hide()` hides the lens. Setting `enabled = false` stops the runtime and releases
its GPU resources, while keeping the plugin API registered. After re-enabling,
call `show()` or `toggle()` to display the lens again.

Unloading the shared object removes the plugin and its API. This is separate
from both visibility and the `enabled` configuration option.

## Limitations and troubleshooting

- Glasscope magnifies the frame Hyprland has already composed. It cannot make an
  application redraw text or images at a higher resolution.
- The current implementation requires OpenGL and an unrotated output.
- Rebuild after a Hyprland upgrade if the plugin reports a version mismatch.
- Check `hyprctl plugin list` and `hyprctl configerrors` if it does not appear.
  Ensure `plugin.glasscope.enabled` is `true`, then use the lens toggle.
- If colours do not appear, check that `color_strength` is above zero and that
  the relevant RGBA alpha is not `00`.
- If RGB text is not copied, check that `/usr/bin/wl-copy` is available.
- The loading examples honour `HYPR_NO_PLUGINS=1` to skip plugin loading.

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
