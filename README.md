# Smart Presentation Builder

A terminal-based presentation builder written in C++. Build slides
interactively, get real content from a locally-running AI, and export to
an interactive HTML slideshow or a genuine PowerPoint (`.pptx`) file — all
with zero external tools and no internet connection required at export
time.

## Table of Contents

- [Quick Start](#quick-start)
- [Features](#features)
- [Architecture](#Architecture)
- [The Menu](#the-menu)
- [Creating Slides](#creating-slides)
- [Exporting](#exporting)
- [Importing](#importing)
- [Templates](#templates)
- [AI Features](#ai-features-local-llm-no-internet-required)
- [How It Works Under the Hood](#how-it-works-under-the-hood)
- [Project Layout](#project-layout)
- [Tests](#tests)
- [Troubleshooting](#troubleshooting)
- [Known Limitations](#Known Limitations)
- [Future Improvements](#Future Improvements)


---

## Quick Start

```
mkdir build && cd build
cmake ..
cmake --build .
./presentation_builder
```

Requires a C++17 compiler and CMake 3.15+. No other dependencies — the zip
library used for `.pptx` export (`miniz`) is vendored in `third_party/miniz`,
so there's nothing else to install.

The build is split into a static library (`presentation_builder_lib`,
everything except `main.cpp`) and a thin app executable that links against
it. The test suite links against the same library, so tests exercise the
real, unmodified implementation rather than a separate copy.

---

## Features

- Interactive terminal interface
- HTML slideshow export
- PowerPoint (.pptx) export
- Import previously exported HTML presentations
- Built-in presentation templates
- Theme system
- Image slides
- Bullet slides
- Custom HTML slides
- Local AI slide generation via Ollama
- Lightweight JSON parser
- HTTP client implemented in C++
- No external presentation libraries

## Architecture

Presentation
│
├── Slide (abstract)
│   ├── TitleSlide
│   ├── ContentSlide
│   ├── BulletSlide
│   ├── ImageSlide
│   └── CustomHTMLSlide
│
├── Exporters
│   ├── HTML Exporter
│   └── PPTX Exporter
│
├── AI
│   ├── HTTP Client
│   ├── JSON Parser
│   └── Ollama Integration
│
└── Templates

## The Menu

Options are grouped by what they do, not just numbered 1-20 in a flat
list:

```
========== Create ==========
1. Title Slide
2. Content Slide
3. Image Slide
4. Bullet Slide
5. Add raw HTML slide
========== Edit ==========
6. Change Theme
7. Move Slide
8. Delete Slide
========== View ==========
9. Summary
10. View All Slides
========== Export ==========
11. Export HTML
12. Export Individual HTML
13. Export PPTX
========== AI ==========
14. Generate Outline
15. Suggest Bullets
16. AI Settings
========== Templates ==========
17. Import HTML presentation
18. Use Built-in Template
19. Save as Template
20. Load Template
0. Exit
```

---

## Creating Slides

Five slide types, all edited by re-running the relevant "Create" option or
using **7. Move Slide** / **8. Delete Slide** to rearrange:

| Type | What it holds |
|---|---|
| Title | A big title + subtitle — typically your first slide |
| Content | A heading + a short paragraph |
| Image | A path to an image file + caption (embedded for real in PPTX export) |
| Bullet | A heading + a list of bullet points |
| Raw HTML | Arbitrary HTML you write yourself, dropped into a styled slide container |

Seven built-in color themes are available from **6. Change Theme**: Dark,
Minimal, Retro, Ocean, Sunset, Forest, Coffee.

---

## Exporting

- **11. Export HTML** — writes an interactive `presentation.html`: one
  slide visible at a time, navigate with the on-screen arrow buttons,
  mouse clicks, or left/right arrow keys. Plain vanilla JS, no
  dependencies, works fully offline straight from a `file://` URL.
- **12. Export Individual HTML** — one static `slide_N.html` file per
  slide (no navigation needed since each file is a single slide).
- **13. Export PPTX** — writes `presentation.pptx`, a real PowerPoint file
  built directly by the program (see [`PptxBuilder`](#how-it-works-under-the-hood)
  below) — no Node.js, no Python, no internet connection needed.

---

## Importing

**17. Import HTML presentation** reopens a previously-exported HTML file
(from either export option above) and reconstructs real, editable slides
from it — title, content, bullets, image, or raw HTML, correctly
recognized and rebuilt as the right slide type. This is designed to
round-trip *this program's own markup*; if a slide doesn't match a
recognized shape (e.g. heavily hand-edited HTML), it's preserved as a
Raw HTML slide rather than dropped.

---

## Templates

- **18. Use Built-in Template** — instantiates one of four ready-made
  multi-slide skeletons: Business Pitch, Project Status Update, Lecture
  Outline, Product Launch.
- **19. Save as Template** — saves your current slide structure to a
  `.template` file (defaults to that extension automatically if you don't
  type one) for reuse later.
- **20. Load Template** — shows the built-in templates *and* any
  `.template` files found in the current directory in one combined list,
  or type a custom file path directly:
  ```
  BUILT-IN TEMPLATES:
  1. Business Pitch - ...
  2. Project Status Update - ...
  3. Lecture Outline - ...
  4. Product Launch - ...

  SAVED TEMPLATES (found in current directory):
  5. weekly_report.template
  6. oop_presentation.template

  Choose a template number, or type a custom file path:
  ```

**After picking a template (18 or 20), you get a choice:**
```
Fill in placeholders (m)anually, or let (a)i generate content for a topic? [m/a]
```
- **Manual** — the template's placeholder text is added as-is, for you to
  edit slide by slide.
- **AI** — give a one-line topic, and the AI regenerates every slide's
  actual content for that topic. The template's section headings (e.g.
  "The Problem", "Key Features") stay as they are — they're already
  generic enough to fit any topic in that template's genre — only the
  bullets/paragraphs underneath get freshly written. Image and Raw HTML
  slides are left untouched either way, since Ollama is a text model and
  can't generate or meaningfully choose an image.

Template files use the same underlying slide-spec shape as the AI outline
feature (type/heading/subtitle/content/bullets), so both features share
one format.

---

## AI Features (local LLM, no internet required)

The program can talk to a local LLM server to draft content for you.
Default target is [Ollama](https://ollama.com) on `localhost:11434`.

1. Install Ollama and pull a model, e.g. `ollama pull llama3.2`
2. Run `ollama serve` (or it may already be running as a background service)
3. In the program, use **16. AI Settings** to point at your host/port/model
   (defaults are usually fine if Ollama is running locally). You can switch
   models here too — e.g. to `qwen2.5:7b` for more reliable structured
   output at the cost of slower generation — as well as raise the timeout
   if generation is timing out on slower hardware.
4. Use **14. Generate Outline** to draft an entire deck from a one-line
   topic, or **15. Suggest Bullets** to fill in one slide at a time. The
   outline generator produces a genuine mix of bullet-list and
   short-paragraph slides (not just bullets everywhere) — it asks the
   model which sections read better as prose versus bullets, and falls
   back to bullets for any section where paragraph generation fails.
   Either way, you review the suggestions and confirm before anything is
   added — nothing is added automatically.

No API keys, no internet connection, no data leaves your machine — the
whole request/response cycle happens over a local HTTP connection to your
own Ollama instance. If Ollama isn't reachable, the program tells you so
and points you at `ollama serve`.

Every blocking AI call (checking connection, generating an outline,
suggesting bullets, AI-filling a template) shows an animated spinner while
it works, so the console never looks frozen during a slow local-model
request.

### Dealing with a model that doesn't follow instructions perfectly

Small local models (especially something like `llama3.2` at the 3B size)
are noticeably less reliable at producing exact JSON shapes than larger
hosted models. Rather than trusting the model blindly, this program
recovers from several real, observed failure patterns instead of just
failing outright:

- **Markdown-fenced replies** (` ```json ... ``` ` wrapping, despite being
  told not to) are stripped before parsing.
- **Bullets returned as object keys** instead of an array (a real llama3.2
  failure mode: `{"first bullet": "", "second bullet": ""}`) are recovered
  automatically — guarded so a *legitimately* structured reply (like a
  title/subtitle object) never gets misread as a pile of unrelated
  bullets.
- **Deeply nested replies** (the array wrapped one or two objects deeper
  than asked) are searched for recursively rather than requiring an exact
  top-level shape.
- Every failure message shows the **actual raw reply** from the model, so
  a problem is always diagnosable rather than a silent "something went
  wrong."
- A failed section gets a short backoff before the next request, since
  hitting an already-struggling model immediately again often just
  repeats the failure.

If you hit a case none of this catches, the error message will include
the model's raw reply — that's exactly the information needed to add
another recovery case.

---

## How It Works Under the Hood

| Component | Role |
|---|---|
| `Theme` | Color palette + CSS generation for a visual theme |
| `Slide` | Abstract base for all slide types (polymorphic interface) |
| `TitleSlide`, `ContentSlide`, `BulletSlide`, `ImageSlide`, `CustomHTMLSlide` | Concrete slide types |
| `Presentation` | Owns the slide deck, current theme, and export logic |
| `PptxBuilder` | Builds a valid `.pptx` (OOXML) file directly from slide data |
| `HttpClient` | Small cross-platform HTTP/1.1 client over raw TCP sockets (POSIX on Linux/macOS, Winsock on Windows) — no libcurl dependency |
| `Json` / `JsonHelpers` | A hand-written JSON parser, plus the tolerant tree-searching logic used to handle inconsistent LLM output |
| `AIAssistant` | Builds prompts, talks to Ollama's `/api/generate`, and turns replies into plain data (`AISlideSpec`) |
| `HtmlImporter` | Reconstructs real `Slide` objects from previously-exported HTML |
| `TemplateManager` | Built-in template skeletons + loading/saving templates as JSON |

**`.pptx` files are just a ZIP of XML text files** (the OOXML format) —
`PptxBuilder` generates that XML directly and zips it using `miniz`, a
small vendored library (source lives in `third_party/miniz/`, compiled
alongside the project — nothing to install separately).

---

## Project Layout

```
include/             Public headers for every class
src/                 Implementations + main()
tests/               Unit tests (see Tests section below)
third_party/miniz/   Vendored zip library (used only by PptxBuilder)
docs/                Deeper architecture notes (see ARCHITECTURE.md)
.github/workflows/   CI: builds + runs the test suite on every push/PR
CMakeLists.txt       Build system
CONTRIBUTING.md      Build/test workflow, code conventions, known pain points
```

---

## Tests

```
cd build && ctest --output-on-failure
```

Each test is a small standalone executable using plain `assert()` — no
test framework dependency, consistent with the rest of the project. CTest
ships with CMake, so nothing extra needs installing.

CI (`.github/workflows/ci.yml`) builds and runs this full suite on Linux,
macOS, **and Windows** on every push and pull request — the Windows
Winsock networking code path in particular had never actually been
compiled anywhere before CI was added, only reviewed by eye.

| Test | Covers |
|---|---|
| `test_json` | The hand-written JSON parser: nested/escaped strings, malformed input, numbers/bools/null, unicode, markdown-fence stripping |
| `test_json_helpers` | The tolerant-parsing logic used to handle inconsistent LLM output — recursive array search, and recovering bullets from object keys (with the guard against misreading a legitimately-structured reply) |
| `test_utils` | `escapeHTML`/`unescapeHTML` round-tripping (including the apostrophe-in-attribute-value case), `trim`, `stripQuotes` |
| `test_theme` | Theme getters, all 7 built-in themes have distinct names, `buildCSS()` actually embeds each theme's own colors |
| `test_slides` | Direct tests of each slide type's `generateHTML()`/`toTemplateJSON()` output and escaping, plus `MAX_BULLETS` enforcement |
| `test_presentation` | Add/delete/move bounds checking and ordering, theme switching, and that exporting an empty presentation is a safe no-op |
| `test_template_manager` | All 4 built-in templates, and loading saved template files (including malformed/unrecognized-type files failing cleanly) |
| `test_http_client` | A self-contained local test server (no Python needed) verifying `HttpClient` against real 200/404/chunked responses and a refused connection |
| `test_html_importer` | All 5 slide types round-tripping through HTML import, including a slide with nested `<div>`s (tests depth-aware block splitting) |
| `test_pptx_roundtrip` | Builds a real `.pptx`, unzips it with miniz's own reader, and asserts the slide XML and relationship files contain expected content for every slide type (title, content, bullets, image found/missing, custom HTML) — the kind of test that would have caught the "every slide is missing its layout relationship" bug found during an earlier PowerPoint "needs repair" investigation |

---

## Troubleshooting

**"AI request failed" / model returns garbage** — check the error message;
it includes the model's actual raw reply. Try a larger/more capable model
via **16. AI Settings** if a small model keeps struggling with structured
output.

**Generation times out** — raise the timeout in **16. AI Settings**; CPU
inference on ordinary hardware can genuinely take a while, especially for
larger models.

**PowerPoint says a file needs repair** — this specific issue (every slide
missing its required slide-layout relationship) was found and fixed; if
you hit a *new* repair prompt, PowerPoint's dialog usually names the exact
part it choked on — that's the fastest way to track down what's wrong.

**A pasted file path doesn't work** — if you copied it via Windows
Explorer's "Copy as path," it includes literal surrounding quotes; the
program strips those automatically, but if something still looks off,
try typing the path directly instead.

## Known Limitations

### Raw HTML slides

1.Custom HTML slides are inserted directly into the exported HTML presentation.
	Because arbitrary HTML and CSS are preserved exactly as provided by the user,
	global CSS rules (for example `body`, `html`, `*`, or generic element
	selectors) may affect the entire presentation instead of only the custom
	slide.
	This is an intentional trade-off to keep the exporter lightweight and avoid
	implementing a full HTML/CSS parser or sandboxing engine.

	For best results:
	- Prefer inline styles or uniquely named CSS classes.
	- Avoid global CSS selectors.
	- Use Custom HTML for small embedded components rather than complete webpages.

2.pptx export does not support images and custom html (an area for future improvement)

## Future Improvements

- Speaker notes
- PDF export
- Optimized pptx export
- Better image positioning
- Embedded fonts
- Animation support
- Markdown slide import
- CSS sandboxing for custom HTML
- Theme editor