# Grid ARV  -  by Kone Labie

Grid ARV (Audio Reactive Visualizer) is a real-time visualizer, available as a **VST3 / AU / (VST2) plugin** and a **standalone app**.
103 visual modes, lyric/text automation, drag-and-drop images and fonts, an image-synthesis (Photism-style) family,
and a GPU pipeline with trails, glow and beat-reactive effects. Built with JUCE 8 + OpenGL.

> **Status:** dependency-free source and installer tests pass. A recent cross-platform CI run exposed a JUCE 8 caret-colour API
> error, now fixed in `Source/PluginEditor.cpp` and guarded by a regression check in `tests/verify_source_wiring.py`.
> A successful native build on all platforms has not yet been confirmed in this environment.

## 1. LOCK. IN.

<img width="400" height="303" alt="porter-robinson-stream" src="https://github.com/user-attachments/assets/6536a211-9843-4221-92aa-e0be8020d6bd" />

## 2. Install

**Recommended for users:** download the direct installer asset from the GitHub Release rather than a source ZIP. One installer lets users choose components in a menu; they do not need to extract an installer from another archive.

| OS | Installer and component choices |
|---|---|
| **Windows** | Run `GridARV-Installer-Windows-Standard.exe` (or the `-VST2` variant for an authorized VST2 build). The component page lets you select the VST3 plug-in, the standalone app, or both. Administrator permission is required to install the system-wide VST3 plug-in. |
| **macOS** | Open `GridARV-Installer-Standard-macOS.pkg` (or the `-VST2` variant for an authorized VST2 build). Choose **Customize** to select VST3, Audio Units (AU), and/or the standalone app. The package is currently unsigned; macOS may require Control-click -> Open and confirmation in Privacy & Security. |
| **Linux** | Run `GridARV-Installer-Standard-Linux.run` in a terminal (or the `-VST2` variant for an authorized VST2 build). Choose VST3, standalone, or both. Installs to your user account (`~/.vst3` and `~/.local/opt/GridARV`) and adds a desktop-menu entry for the standalone app. You can also run it non-interactively with `--vst3`, `--standalone`, or `--all`. |

The source/build ZIPs and the older per-platform install scripts remain available for manual installation. Release pages attach direct installers for each OS, alongside the ZIP archives. Their filenames include `Standard` by default or `VST2` for a separately authorized VST2 build.

**FL Studio:** *Options -> Manage plugins -> Find plugins*, then add *Grid ARV* as an effect on a mixer insert (e.g. the Master).
**Ableton Live:** *Preferences -> Plug-ins -> Rescan*, drop it on any track (VST3 on Win/Mac, AU on Mac). Click the wrench icon / device title to show the window.
The plugin passes audio through untouched. In Ableton you can also use the **Sidechain** input (set *Input = Sidechain* in the panel) to visualize a different track.

## 3. Standalone app

Listens to any **audio input**. Press *Audio settings...* in the panel to pick the device (remembered next launch).
To visualize what your computer is playing, route it into an input:
- **Windows:** enable *Stereo Mix* (Sound settings -> Recording) or install a virtual cable (VB-Cable / Voicemeeter).
- **macOS:** install BlackHole (or Loopback) and use it as the input.
- **Linux:** choose the PulseAudio/PipeWire *monitor* source.

Keyboard shortcuts can be re-bound in **Settings → Key Bindings**. Defaults are **F** for fullscreen (standalone only), **F8** for clean output, **R** for random mode, **Tab** to show/hide controls, and **Left/Right Arrow** for previous/next mode. Clean output hides the settings panel and floating UI button for capture. The visualization itself continues rendering; any optional visual text/lyrics are still part of the artwork.

**OBS tip:** in the standalone app, select a *Window Capture* source, switch Grid ARV to clean output with F8, then use fullscreen if desired. Reopen the controls with F8 after capture setup. If you do not want the visual lyric/text overlay in the stream, turn off *Show visual text* before entering clean output.

## 4. Controls

- **Key Bindings** under **Settings** lets you reassign fullscreen (standalone), clean output, random mode, show/hide controls, and previous/next mode. Duplicate shortcut assignments are rejected.
- *Auto-cycle (beats)*: switch to the next mode every N detected beats.
- *Quality* (50 / 75 / 100 %): internal render resolution. Lower it if your GPU struggles (raymarched and fractal modes are the heaviest).
- The menu order is **Status** (pinned first), always-visible mode controls, **Motion & Colour**, **Image**, **Text**, and **Settings**. Image contains nested **Framing & Orientation**; Settings contains **Timing & Advanced**, **Window & Stream Capture**, and **Key Bindings**.
- *Trails / Trail zoom / Trail rotate / Glow / Aberration*: the feedback + post-processing stage that makes most modes look good.

### Images
Drag **PNG / JPG / GIF / BMP** files onto the window (or use *Load image...* in **Image**). Files are copied to
`<app data>/GridARV/Images` so projects keep working. Use them as:
- the **Image FX** modes (pulse, ripple, RGB split, swirl, mosaic, glitch, spectrum wave, kaleido, luma-spectrum, edge glow), or
- a **background under any mode** (*Background mix*), optionally cycling every N beats.

Open **Image → Framing & Orientation** for shared image controls used by both background images and Image FX:
- **Fit mode:** *Cover* fills the display and crops the long edge; *Contain* shows the whole image without cropping; *Stretch* fills the display without preserving the image aspect ratio.
- **Scale:** zoom the image from 0.1× to 4×; **Rotation** turns it in degrees; **Flip horizontally/vertically** mirror it; **Horizontal/Vertical position** moves it across the display. *Reset image framing* restores defaults.

### Text and lyrics
Type in the text box, or drop a file onto the window:
- **Plain text**: one slide per line (`|` makes a line break inside a slide).
- **LRC** (`[01:23.45] line`) and **SRT**: timed lyrics.

Which line is shown (priority order):
1. *Line* parameter > 0: that line (automate it in your DAW to trigger lines manually).
2. Timed lyrics: follows the **DAW playhead** (*Lyrics follow DAW playhead*), or the internal clock (*Clock: Play/Stop/Reset*, used in standalone). *Lyric offset* nudges the timing.
3. *Advance (beats)* > 0: next line every N beats.
Animations: Static, Fade, Pop, Slide, Typewriter, Beat Shake, Glitch, Wobble. Colour: white, palette gradient, palette solid, black.
Drag **TTF / OTF** fonts onto the window, or pick any installed system font.

### DAW automation
Every setting is a host parameter: visual mode (shows the mode *name*), palette, sensitivity, trails, glow, text line, text size, image slot, etc.
Automate *Visual Mode* to change visuals on cue and *Text Line* to trigger lyrics.

## 5. The 103 modes

- **Spectrum** (14): Classic Bars, Fine Bars, Mirror Bars, LED Stack, LED Mirror, Chunky Blocks, Wide Wall, Radial Burst, Radial Dual, Sunburst Fill, Halo Line, Spin Ring, Triple Arc Meter, Level Bars
- **Waveform** (8): Scope Line, Scope Mirror, Scope Stereo, Scope Layers, Scope Circle, Phase Scope, Phase Mid/Side, Phase Thin
- **Geometric** (20): Warp Tunnel, Square Tunnel, Hex Tunnel, Twist Tunnel, Checker Tunnel, Triangle Rings, Square Rings, Hexagon Rings, Pentagon Rings, Hypno Spiral, Vortex, Spiral Checker, Hex Grid, Honeycomb, Light Rays, Spiral Rays, Laser Fan, Beat Rings, Beat Squares, Beat Hexagons
- **Fractal** (6): Kali Bloom, Kali Lace, Kali Deep, Julia Pulse, Ship Julia, Cubic Julia
- **Organic** (17): Plasma Classic, Plasma Liquid, Plasma Tight, Metaballs, Lava Lamp, Blob Swarm, Cell Edges, Cell Fill, Crystals, Nebula, Smoke, Aurora Flow, Fire Wall, Plasma Flames, Ripples, Interference, Water Drops
- **Space** (9): Starfield, Hyperspace, Comet Rain, Orbit Orbs, Spiral Galaxy, Audio Sphere, Audio Torus, Audio Cube, Orb Fusion
- **Retro** (6): Synthwave, Neon Horizon, Digital Rain, Data Stream, Ridgelines, Pulsar Waves
- **Particles** (5): Dot Matrix, Halftone Hex, Polar Dots, Beat Fireworks, Confetti Burst
- **Image Synthesis** (8): Spectrogram, Spectrogram Mirror, Spectrogram Vertical, Spectrogram Radar, Additive Waves, Moire Synth, Radial Synth, Crystal Synth
- **Image FX** (10): Image Pulse, Image Ripple, Image RGB Split, Image Swirl, Image Mosaic, Image Glitch, Image Spectrum Wave, Image Mirror Tiles, Image Luma Spectrum, Image Edge Glow

## 6. VST2 (optional and restricted)

Standard builds use JUCE 8.0.4 and produce VST3 plus the standalone app. VST2 is a separate opt-in build using JUCE 7.0.12 and a privately installed SDK. The SDK is never stored in this repository or uploaded as an artifact. See the **Optional VST2 builds** section below for the runner setup, installer behaviour, and legal gate.

## 7. Local build (optional)

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```
Linux needs: `libasound2-dev libfreetype6-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev libgl1-mesa-dev`.

## 8. Architecture

```
Source/
  PluginProcessor.*   audio thread: copies samples into a lock-free ring buffer (no allocation), reads host transport; state, parameters, asset library
  AudioAnalyzer.h     GL thread: FFT -> 512 log-frequency bins, bass/mid/treble, level, beat detection, waveform (JUCE-independent, unit-tested)
  VisualRenderer.*    OpenGL: mode shader -> scene FBO -> trails FBO -> final pass (glow/aberration) -> text quad
  Shaders.h           GLSL for the 30 shader families (103 modes = family + preset parameters, see VisualModes.h)
  VisualModes.h       the mode table - add a row to add a mode
  TextEngine.h        lyrics parsing (plain/LRC/SRT), line selection, text-to-texture
  PluginEditor.*      the OpenGL view + control panel + drag & drop
  StandaloneApp.cpp   standalone window, audio device handling
```
Performance notes: shaders compile lazily the first time a mode is used; audio is uploaded as one 512x4 texture per frame;
static-image resampling/texture upload happens only when the selected image changes; animated GIF frames are decoded/resampled on a single background worker and uploaded on the OpenGL thread only when their frame delay elapses (frames and dimensions are capped to bound memory); large static images are capped at 2048 px per side using high-quality resampling;
the image transform uses uniforms and precomputed rotation values; rendering stops completely when the plugin window is closed; vsync is on; render scale is adjustable.

Run the dependency-free checks with `python tests/verify_source_wiring.py` and `python tests/verify_installer_packaging.py`. On Linux, `python tests/verify_linux_installer.py` builds a sample self-extracting package and tests both install choices. These checks do not replace a native JUCE build, OS-native Windows/macOS installer build, or a live GPU test.

### Adding a mode
Add a row to `kModes` in `VisualModes.h` using an existing family and different parameters (`p[4]`, `q[4]`; meanings are in the family's shader comment/code in `Shaders.h`).
For a new family: add an `enum Base` entry, a GLSL `vec3 render(vec2 uv, vec2 st)` string and its slot in `kModeSource`.

## 9. If the first build fails

Open the failed job in the Actions tab, copy the first `error:` lines and send them back; compile errors from a never-compiled
code base are usually one-line fixes. GLSL errors are not build errors: they appear in the panel's status line at runtime
("SHADER ERROR: ...") and affect only that family - the rest keeps working.

## 10. Licensing notes

JUCE is dual-licensed (AGPLv3 or a JUCE commercial/free-tier license with revenue limits). If you plan to sell this plugin
closed-source you need the appropriate JUCE license - see juce.com/legal. VST is a trademark of Steinberg; VST3 is MIT/GPLv3 licensed via JUCE.


## Optional VST2 builds (licensed distributors only)

The single `.github/workflows/build.yml` workflow replaces the former separate VST2 workflow. Standard builds remain on GitHub-hosted runners and produce VST3 plus the standalone app. VST2 is opt-in from **Actions → Build → Run workflow**; the installer only shows the VST2 choice when that build actually produced a VST2 bundle.

**Legal gate:** Steinberg's current developer licensing guidance says a VST2 plug-in may only be distributed if the developer already has the required written VST2 distribution permission under a licence signed before October 2018. Having a copy of the SDK is not sufficient. Do not enable VST2 distribution unless GridARV is covered by that permission. See the [Steinberg VST 2 licensing guidance](https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Licensing.html).
Authorized VST2 packages also include `VST2-DISTRIBUTION-NOTICE.txt` with the required Steinberg attribution and licensing reminder. Steinberg's guidance also requires distributors to mention Steinberg Media Technologies GmbH in the plug-in's about box and/or documentation and follow its VST usage guidelines. The notice is installed with the VST2 component; do not remove it from a VST2 package.

To build/package VST2, an authorized distributor must (1) set repository variable `VST2_DISTRIBUTION_AUTHORIZED` to `true`, (2) tick both `build_vst2` and `confirm_vst2_distribution_rights` when manually dispatching the workflow, and (3) provide self-hosted Windows, macOS, and Linux runners carrying the label `gridarv-vst2-sdk`. On each runner, set `VST2_SDK_PATH` to a local SDK directory **outside the checked-out repository**. Do not commit, cache, or upload the SDK or restricted SDK headers. The workflow publishes only the compiled plug-in binary and installer when the VST2 build is explicitly enabled. If you do not have the required written permission, keep VST2 disabled; a checkbox or repository variable does not itself grant a licence. Standard VST3/standalone builds do not need this setup.

## Animated GIF images

GIFs are decoded into timed frames when selected as an image asset. Parsing, composition, and resampling run on a single lazily-created background worker; texture uploads stay on the OpenGL thread. Frames preserve delay/disposal metadata and advance according to each frame's delay. Memory is bounded for large animations; very long animations may use fewer displayed frames to reduce memory use. This feature is source-tested, but should still be verified in a native build and live OBS capture.
