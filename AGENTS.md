# TerQuiz — agent guide

## Instructions

- Do not run the binary.

## Build & run

```sh
make          # build the terquiz binary
make run      # build + run
make clean    # remove build/ and binary
```

The binary needs ncursesw at runtime — see Makefile `LIBS` for the exact `.so` paths (`libncursesw` for UTF-8 support).  
No test, lint, or typecheck infrastructure exists.

## Project structure

- **`src/`** — 11 `.c`/`.h` modules, single binary, entrypoint in `main.c`.
- **`data/`** — 26 tab-separated files. Master files (`{source}.txt`) have `id\tcommand\tcategory\tdifficulty`. Translation files (`{source}-{lang}.txt`) have `id\tdescription`. Lang codes: `eng`, `fi`.
- **`build/`** — `.o` artifacts (checked in).
- **`*.py`** — data conversion helpers (`convert.py`, `fix_ids.py`, `translate_fi.py`). Not part of the build.

## Architecture

| Module | Role |
|---|---|
| `ui` | ncurses wrapper — colors, rects, menus, input |
| `data` | Question model (`Pool`/`Entry`), TSV loading, category/source filtering |
| `game` | Quiz loop, scoring, results |
| `settings` | Persisted user prefs (categories, sources, modes, timer, lives, language) |
| `progress` | Per-command mastery tracking, stored in `~/.terquiz/progress` |
| `difficulty` | Adaptive 20-level difficulty, requires 80% accuracy + 3 mastered to unlock next level |
| `str` | i18n string loader — string IDs enum in `str.h`, loaded from `data/strings-{lang}.txt` |
| `topics` | Category/source selection UI |
| `utils` | Misc helpers |

## Key conventions

- **C11** (`-std=c11`), strict flags (`-Wall -Wextra -Wpedantic`), no compiler warnings tolerated.
- **Settings** persisted to `~/.terquiz/settings` (see `settings.c`). **Progress** persisted to `~/.terquiz/progress`.
- **8 data sources** flagged as bitmask (`SOURCE_CORE` = 1, ..., `SOURCE_MACOS` = 128).
- **11 categories** (`CAT_FILE` 0–`CAT_REDIR` 10), also a bitmask.
- **Input mode is automatic**: show definition → type the command. Show command → pick definition (multiple choice). No separate `input_mode` setting.
- **Retry wrong answers**: wrong first-attempt answers are re-asked until correct. Scoring is first-attempt only. Retries are for learning, shown with a `[RETRY]` tag and no score/lives impact.
- **Inline results**: previous question's result is shown at the top of the next question's screen. No separate full-screen feedback screen between questions.
- **2 languages**: `LANG_ENGLISH` (0), `LANG_FINNISH` (1).
- **Mastery threshold**: 5 consecutive correct answers = mastered.

## Data format notes

Master file columns: `id\tcommand\tcategory\tdifficulty` (difficulty 1–20).  
Under the hood, difficulty level follows TES model: 1-4 novice, 5-8 apprentice, 9-12 expert, 13-16 journeyman, 17-20 master.
Translation files: `id\tdescription`.  
The `id` is a stable slug (see `SPECIAL_CMDS` dicts in `convert.py`/`fix_ids.py` for special-character mappings).
