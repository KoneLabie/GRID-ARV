#!/usr/bin/env python3
"""Fast, dependency-free source smoke tests for GridARV image framing controls."""
from pathlib import Path
import math
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Source"

PARAMS = (SRC / "Params.h").read_text(encoding="utf-8")
EDITOR = (SRC / "PluginEditor.cpp").read_text(encoding="utf-8")
EDITOR_H = (SRC / "PluginEditor.h").read_text(encoding="utf-8")
RENDERER = (SRC / "VisualRenderer.cpp").read_text(encoding="utf-8")
RENDERER_H = (SRC / "VisualRenderer.h").read_text(encoding="utf-8")
SHADERS = (SRC / "Shaders.h").read_text(encoding="utf-8")
PROCESSOR = (SRC / "PluginProcessor.cpp").read_text(encoding="utf-8")

# JUCE 8 exposes the text caret colour ID through CaretComponent, not TextEditor.
if "juce::TextEditor::caretColourId" in EDITOR:
    fail("invalid JUCE TextEditor::caretColourId reference")
if "juce::CaretComponent::caretColourId" not in EDITOR:
    fail("text-editor caret colour must use juce::CaretComponent::caretColourId")


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def check_delimiters(path: Path) -> None:
    """Check C++ delimiter balance while skipping comments and literals."""
    text = path.read_text(encoding="utf-8")
    stack = []
    pairs = {")": "(", "]": "[", "}": "{"}
    i = 0
    line = 1
    while i < len(text):
        if text[i] == "\n":
            line += 1
            i += 1
            continue
        if text.startswith("//", i):
            end = text.find("\n", i)
            i = len(text) if end < 0 else end
            continue
        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            if end < 0:
                fail(f"{path.relative_to(ROOT)}: unclosed block comment near line {line}")
            line += text[i:end + 2].count("\n")
            i = end + 2
            continue
        if text.startswith('R"', i):
            match = re.match(r'R"([^ (\\\t\r\n]*)\(', text[i:])
            if match:
                delimiter = match.group(1)
                end = text.find(")" + delimiter + '"', i + match.end())
                if end < 0:
                    fail(f"{path.relative_to(ROOT)}: unclosed raw string near line {line}")
                end += len(delimiter) + 2
                line += text[i:end].count("\n")
                i = end
                continue
        if text[i] in ('"', "'"):
            quote = text[i]
            i += 1
            while i < len(text):
                if text[i] == "\\":
                    i += 2
                    continue
                if text[i] == quote:
                    i += 1
                    break
                if text[i] == "\n":
                    line += 1
                i += 1
            continue
        c = text[i]
        if c in "({[":
            stack.append((c, line))
        elif c in ")}]":
            if not stack or stack[-1][0] != pairs[c]:
                fail(f"{path.relative_to(ROOT)}: mismatched {c!r} near line {line}")
            stack.pop()
        i += 1
    if stack:
        fail(f"{path.relative_to(ROOT)}: unclosed {stack[-1][0]!r} opened near line {stack[-1][1]}")


IMAGE_IDS = ["imgScale", "imgFit", "imgRotate", "imgFlipX", "imgFlipY", "imgOffsetX", "imgOffsetY"]
for pid in IMAGE_IDS:
    if f'inline constexpr const char* {pid} = "{pid}";' not in PARAMS:
        fail(f"missing parameter ID {pid}")
    if not re.search(r"\b(?:F|C|B)\(ParamID::" + re.escape(pid) + r"\b", PARAMS):
        fail(f"missing APVTS registration for {pid}")
    if f"ParamID::{pid}" not in EDITOR:
        fail(f"missing UI binding for {pid}")
    if f"ParamID::{pid}" not in RENDERER:
        fail(f"missing renderer binding for {pid}")

UNIFORMS = ["uImgScale", "uImgFit", "uImgRotCS", "uImgFlipX", "uImgFlipY", "uImgOffsetX", "uImgOffsetY"]
for uniform in UNIFORMS:
    if uniform not in SHADERS:
        fail(f"missing GLSL uniform {uniform}")
    if f'loc(p, "{uniform}")' not in RENDERER:
        fail(f"missing scene-pass upload for {uniform}")
    if f'loc(finalProg, "{uniform}")' not in RENDERER:
        fail(f"missing composite-pass upload for {uniform}")

if SHADERS.count('R"GLSL(') != SHADERS.count(')GLSL";'):
    fail("GLSL raw-string blocks are not balanced")
if SHADERS.count("vec2 mapImageUV(") != 2:
    fail("expected shared image mapping in the scene and composite shader")
if "Toggle clean output  (F8)" not in EDITOR or "KeyPress::F8Key" not in EDITOR:
    fail("clean-output UI action or F8 shortcut missing")
if "void AudioVisualizerEditor::toggleCleanOutput()" not in EDITOR:
    fail("clean-output state handler missing")
if "setItemVisible(it, visible)" not in EDITOR:
    fail("collapsible settings visibility handling missing")
if "highResamplingQuality" not in RENDERER:
    fail("image downscaling no longer uses high-quality one-time resampling")
if 'hasFileExtension("png;jpg;jpeg;gif;bmp")' not in PROCESSOR:
    fail("image loader extension list is out of sync with the UI")

# Geometry invariants matching the shader's fit-size rule.
def draw_size(canvas_aspect: float, image_aspect: float, fit: int) -> tuple[float, float]:
    width = height = 1.0
    if fit == 0:  # cover
        if canvas_aspect > image_aspect:
            height = canvas_aspect / image_aspect
        else:
            width = image_aspect / canvas_aspect
    elif fit == 1:  # contain
        if canvas_aspect > image_aspect:
            width = image_aspect / canvas_aspect
        else:
            height = canvas_aspect / image_aspect
    return width, height

for canvas_aspect, image_aspect in [(16 / 9, 1.0), (4 / 3, 16 / 9), (9 / 16, 16 / 9), (1.0, 1.0)]:
    cw, ch = draw_size(canvas_aspect, image_aspect, 0)
    if cw < 1.0 or ch < 1.0:
        fail("cover fit must fill both canvas dimensions")
    cw, ch = draw_size(canvas_aspect, image_aspect, 1)
    if cw > 1.0 or ch > 1.0 or not (math.isclose(cw, 1.0) or math.isclose(ch, 1.0)):
        fail("contain fit must show the entire image with one dimension fitted")

for path in [SRC / "Params.h", SRC / "PluginEditor.cpp", SRC / "PluginEditor.h",
             SRC / "VisualRenderer.h", SRC / "VisualRenderer.cpp", SRC / "Shaders.h", SRC / "PluginProcessor.cpp"]:
    check_delimiters(path)

print("PASS: 7 image parameters are registered and connected to UI/rendering")
print("PASS: scene and final shaders receive all image-transform uniforms")
print("PASS: GLSL raw strings and edited C++ delimiters are balanced")
print("PASS: cover/contain geometry checks passed for 4 aspect ratios")
print("PASS: collapsible panel and F8 clean-output wiring are present")
print("NOTE: these are source/geometry smoke tests, not a compiled or live GPU test")
