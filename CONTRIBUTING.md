# Contributing

## Build & test

```
mkdir build && cd build
cmake ..
cmake --build .
ctest --output-on-failure
```

CI (`.github/workflows/ci.yml`) runs exactly this on Linux, macOS, and
Windows for every push and pull request. A PR that doesn't build or pass
`ctest` on all three platforms won't merge cleanly.

## Project structure

See the [README's "How It Works Under the Hood"](README.md#how-it-works-under-the-hood)
section for the component table, and [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
for a deeper technical walkthrough of the trickier parts (the AI recovery
pipeline, the OOXML relationship model, cross-platform networking).

## Code conventions

- One class per header/source file pair (`Foo.h` + `Foo.cpp`), matching
  the class name.
- Every public method gets a doc comment explaining *why*, not just what
  - especially anywhere behavior isn't obvious from the signature (e.g.
  "returns nullptr if X, because Y").
- No new external dependencies without a real reason. This project
  hand-writes its own HTTP client, JSON parser, and vendors `miniz`
  specifically to avoid pulling in libcurl/nlohmann-json/a full zip
  library for what turned out to be a modest amount of code. Match that
  bar before reaching for a new dependency.
- New logic worth checking gets a test. Add a `tests/test_X.cpp` (plain
  `assert()`, no framework - see any existing test for the pattern) and
  register it in `tests/CMakeLists.txt`.

## Known architectural pain points (read before extending these areas)

**The `Slide` hierarchy has a real fan-out problem.** Every export
target (HTML, PPTX, template JSON) is a pure-virtual method that all 5
slide subclasses must implement. Adding a **new slide type** costs one new
class. Adding a **new export format** costs touching all 5 existing
classes. This has already happened 3 times (HTML, PPTX, template JSON) and
will happen again for anything else added later (a Markdown export, say).
A capability-based redesign (one `SlideData` tagged-union struct instead
of 5 subclasses, with each export format as a single free function
switching on `type`) was proposed and would fix this, but hasn't been
done - it's a genuinely invasive rewrite (~15 files) and deserves its own
PR, not to be folded into an unrelated change. If you're about to add a
6th thing every slide needs to do, that's the signal to finally do this
refactor instead of adding a 6th virtual method.

**The AI layer assumes small local models will misbehave, and recovers
rather than fails.** `AIAssistant.cpp` and `JsonHelpers.cpp` have several
non-obvious recovery branches (bullets returned as object keys, replies
wrapped in markdown fences, arrays nested deeper than asked). Each one
exists because a real local model actually did that - they're not
defensive-programming-for-its-own-sake. If you're touching this code, read
the comments before deleting a branch that looks unnecessary; it probably
isn't. See `docs/ARCHITECTURE.md` for the full failure-mode history.

**`HtmlImporter` only round-trips *this program's own* export markup.** It
is not a general HTML parser and isn't trying to be - it recognizes
specific, exact tag shapes (`<h1>`+`<h3>` for a title slide, etc.) and
falls back to a raw Custom HTML slide for anything else. Don't extend it
to be a general-purpose HTML parser; that's a much bigger scope than this
project needs.
