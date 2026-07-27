# Architecture Notes

This covers the parts of the codebase where the *why* isn't obvious from
reading the code alone - the reasoning trail behind decisions that came
out of real bugs, not just up-front design.

## The Slide hierarchy, and its known cost

`Slide` is an abstract base with 5 concrete subclasses (`TitleSlide`,
`ContentSlide`, `BulletSlide`, `ImageSlide`, `CustomHTMLSlide`), each
implementing:

- `display()` - console output
- `generateHTML()` - HTML export
- `addToPPTX()` - PowerPoint export
- `toTemplateJSON()` - template save/load and AI-fill

Every new "thing every slide can do" is a new pure-virtual method touching
all 5 classes. This is a textbook instance of what's sometimes called the
"expression problem": easy to add a new slide *type* (one new class), hard
to add a new *operation* (touch every existing class). Given this project
has added HTML, PPTX, and template-JSON export in sequence - three
separate times paying that fan-out cost - a capability-based design (a
single `SlideData` struct with a `type` tag, and each export as one free
function switching on it) would fix this at the cost of losing
per-subclass encapsulation. Proposed, not yet done - see CONTRIBUTING.md.

## The AI recovery pipeline: a history of real failures, not speculative robustness

Every recovery branch in `AIAssistant.cpp`/`JsonHelpers.cpp` exists
because a real local model (`llama3.2`, specifically, at the 3B size)
actually produced that exact malformed output during development. In
order encountered:

1. **Whole-outline-in-one-shot was unreliable.** Asking for a deeply
   nested array of full slide objects in a single request, the model
   would just give up and return one object instead of an array. Fix:
   decompose into a simple flat "title + headings" request, then one
   small request per section - each individually much easier for a small
   model to get right.
2. **Bullets returned as object keys.** Asked for `["bullet one", "bullet
   two"]`, the model sometimes returned `{"bullet one": "", "bullet two":
   ""}` instead. `JsonHelpers::recoverBulletsFromObjectKeys` recovers
   this - but only after checking the object doesn't have a recognizable
   field name like `"title"` first, since a legitimately-structured reply
   must never be misread as a pile of unrelated bullets.
3. **Markdown fences despite instructions not to.** Some replies arrive
   wrapped in ` ```json ... ``` ` even with `"format":"json"` set and
   explicit "no markdown" instructions. `stripMarkdownFence()` in
   `Json.cpp` strips this once, centrally, in `queryJSON()`, so every
   caller benefits automatically rather than needing its own fence-check.
4. **Genuine timeouts on CPU inference.** A 3B model on ordinary hardware
   can take longer than expected, especially mid-way through a multi-call
   outline generation. The per-section retry loop includes a short backoff
   after a failure specifically to give a possibly-wedged model room to
   recover before the next request hits it again.

**Why show the raw reply in every error message?** Early versions surfaced
generic messages like "wasn't a usable JSON array." That's nearly useless
for diagnosing *which* of the above (or a new, unseen) failure mode
occurred. Every failure path now includes a truncated raw reply
specifically so a new failure pattern is diagnosable from the error
message alone, without needing to reproduce it with logging added.

## The PPTX / OOXML relationship model

A `.pptx` is a ZIP archive of XML text files (OOXML). The single most
important, least obvious rule: **every slide must declare a relationship
to its slide layout**, in that slide's own `_rels/slideN.xml.rels` file -
even a slide with no images and nothing else to relate to. This isn't
optional decoration; it's how a slide "knows" which layout it belongs to.

This was missed in an early version of `PptxBuilder` - slides without
images got no `.rels` file at all. The file still opened in **LibreOffice**
(which defaults to some layout when the relationship is missing) but
Microsoft PowerPoint validates this strictly and offered to "repair" the
file, silently discarding whatever it couldn't validate in the process.
`test_pptx_roundtrip.cpp` now asserts this relationship exists for every
slide type specifically so this can't regress silently again - it was only
caught originally by manually opening the file in real PowerPoint and
seeing the repair prompt, which isn't something CI can do.

**Verification approach, given no real PowerPoint is available in CI or
this sandbox:** the file was validated by generating a known-good
reference `.pptx` with `python-pptx` (a library that produces files real
PowerPoint accepts) and using `opc-diag` to structurally compare our
output against it, part by part. Not a substitute for testing in real
PowerPoint, but the strongest verification available without it.

## Cross-platform networking

`HttpClient` is a from-scratch HTTP/1.1 client over raw TCP sockets (no
libcurl), because the only thing it needs to do is talk to a local Ollama
server. POSIX sockets (Linux/macOS) and Winsock (Windows) are different
APIs for the same job, hidden behind `#ifdef _WIN32` in `HttpClient.cpp`
(and duplicated, deliberately, in `tests/test_http_client.cpp`'s
self-contained test server - see that file's header comment for why it
doesn't just shell out to Python instead).
