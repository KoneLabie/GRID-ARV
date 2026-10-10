# Grid ARV  -  by Kone Labie

Grid ARV (Audio Reactive Visualizer) is a real-time visualizer, available as a **VST3 / AU / (VST2) plugin** and a **standalone app**.
103 visual modes, lyric/text automation, drag-and-drop images and fonts, an image-synthesis (Photism-style) family,
and a GPU pipeline with trails, glow and beat-reactive effects. Built with JUCE 8 + OpenGL.

> **Status:** the audio analysis (FFT, beat detection) is unit-tested, but this project has **not yet been compiled or
> run on a real machine** by its author (no JUCE/GPU in the authoring environment). Expect to fix a few compiler errors or
> shader tweaks on the first CI run. Section "If the first build fails" explains how.

## Install

| OS | Do this |
|---|---|
| **Windows** | Unzip, right-click `install_windows.bat` -> *Run as administrator*. (Copies the VST3 to `C:\Program Files\Common Files\VST3`.) Standalone: `Standalone\Grid ARV.exe`. |
| **macOS** | Unzip, run `install_mac.sh` in Terminal (`bash install_mac.sh`). Installs VST3 + AU and removes the quarantine flag (builds are unsigned). Standalone app is in `Standalone/`. |
| **Linux** | `bash install_linux.sh` (installs to `~/.vst3`). Standalone binary in `Standalone/`. |

**FL Studio:** *Options -> Manage plugins -> Find plugins*, then add *Grid ARV* as an effect on a mixer insert (e.g. the Master).
**Ableton Live:** *Preferences -> Plug-ins -> Rescan*, drop it on any track (VST3 on Win/Mac, AU on Mac). Click the wrench icon / device title to show the window.
The plugin passes audio through untouched. In Ableton you can also use the **Sidechain** input (set *Input = Sidechain* in the panel) to visualize a different track.

## Standalone app

Listens to any **audio input**. Press *Audio settings...* in the panel to pick the device (remembered next launch).
To visualize what your computer is playing, route it into an input:
- **Windows:** enable *Stereo Mix* (Sound settings -> Recording) or install a virtual cable (VB-Cable / Voicemeeter).
- **macOS:** install BlackHole (or Loopback) and use it as the input.
- **Linux:** choose the PulseAudio/PipeWire *monitor* source.

`F` or *Fullscreen* = fullscreen, `Esc` leaves it.

## Controls

- **Tab** or double-click the view: hide/show the panel. **Left/Right**: previous/next mode. **R**: random mode.
- *Auto-cycle (beats)*: switch to the next mode every N detected beats.
- *Quality* (50 / 75 / 100 %): internal render resolution. Lower it if your GPU struggles (raymarched and fractal modes are the heaviest).
- *Trails / Trail zoom / Trail rotate / Glow / Aberration*: the feedback + post-processing stage that makes most modes look good.

### Images
Drag **PNG / JPG / GIF** files onto the window (or *Load image...*). Files are copied to
`<app data>/GridARV/Images` so projects keep working. Use them as:
- the **Image FX** modes (pulse, ripple, RGB split, swirl, mosaic, glitch, spectrum wave, kaleido, luma-spectrum, edge glow), or
- a **background under any mode** (*Background mix*), optionally cycling every N beats.

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

## The 103 modes

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

## What about VST2?

JUCE 8 dropped VST2 and Steinberg no longer licenses the VST2 SDK, so VST2 is **opt-in** and the SDK is **not included**.
If you own a VST2 license: in a *private* copy of the repo add the SDK as `vst2sdk/` (so `vst2sdk/pluginterfaces/vst2.x/aeffect.h` exists)
and run the *Build with VST2 (manual)* workflow. It builds with JUCE 7.0.12 and adds a `VST/` folder to the zips.
Local build: `cmake -B build -DENABLE_VST2=ON -DVST2_SDK_PATH=/path/to/vst2sdk -DJUCE_TAG=7.0.12`.

## Architecture

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
rendering stops completely when the plugin window is closed; vsync is on; render scale is adjustable.

### Adding a mode
Add a row to `kModes` in `VisualModes.h` using an existing family and different parameters (`p[4]`, `q[4]`; meanings are in the family's shader comment/code in `Shaders.h`).
For a new family: add an `enum Base` entry, a GLSL `vec3 render(vec2 uv, vec2 st)` string and its slot in `kModeSource`.
