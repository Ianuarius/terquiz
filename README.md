# TerQuiz

**TerQuiz** is a terminal-based Linux command quiz that tests your knowledge of shell commands, terminal hotkeys, redirection operators, and package managers across multiple distributions. Built in C with ncursesw for UTF-8 support.

## Features

- **8 data sources**: core, hotkeys, builtins, redirection, Debian, RHEL, Arch, macOS
- **11 categories**: file operations, text processing, networking, process management, system information, users & permissions, package management, development tools, shell basics, terminal hotkeys, redirection/pipes
- **Adaptive difficulty**: 20 levels (novice → master), requires 80% accuracy + 3 mastered commands to unlock the next level
- **Dual quiz modes**: guess the command from a definition (type it) or guess the definition from a command (multiple choice)
- **Timer & lives**: optional per-question timer and life count
- **Practice mode**: no penalties, show answers
- **Progress tracking**: per-command mastery stored in `~/.terquiz/progress`
- **Retry system**: wrong answers are re-asked until correct; scoring is first-attempt only
- **i18n**: English and Finnish UI translations
- **Inline results**: previous question's result shown at the top of the next screen

## Build & run

```sh
make          # build the terquiz binary
make run      # build + run
make clean    # remove build/ and binary
```

**Dependencies**: `libncursesw` (UTF-8 ncurses). On Debian/Ubuntu:

```sh
sudo apt install libncurses-dev
```

The Makefile links directly to the `.so` files — adjust `LIBS` for your system if needed.

## Usage

Run `terquiz` from the build directory. The main menu offers:

| Option | Description |
|---|---|
| **Play** | Start a quiz with the current settings |
| **Topics** | Select which categories and data sources to include |
| **Settings** | Adjust mode, quiz length, timer, lives, practice mode, language |
| **Progress** | View per-command streak and mastery status |

### Quiz flow

1. Questions draw from the selected topics, filtered by your current difficulty level
2. Depending on the mode, you either type the command or pick the correct definition
3. Correct first-attempt answers reward points and build streaks
4. Wrong answers are re-asked (tagged `[RETRY]`) without score/lives impact
5. A results screen shows score, accuracy, best streak, and a letter grade

### Settings

- **Mode**: guess the command, guess the definition, or mixed
- **Quiz length**: 10, 20, 50, or endless
- **Timer**: per-question time limit (5/10/15/30 seconds, or off)
- **Lives**: 1, 3, 5, or unlimited
- **Practice mode**: no penalties, answers shown
- **Language**: English or Finnish (Suomi)

## Data format

All data files are tab-separated (`.txt`) in `data/`.

### Master files

`{source}.txt` — one per data source. Columns: `id\tcommand\tcategory\tdifficulty`.

```
ls	ls	file	1
ls-a	ls -a	file	3
```

- `id` — stable slug (lowercase, hyphens for special chars)
- `command` — the shell command or keybinding
- `category` — one of the 11 category IDs
- `difficulty` — 1–20 (1–4 novice, 5–8 apprentice, 9–12 expert, 13–16 journeyman, 17–20 master)

### Translation files

`{source}-{lang}.txt` — for each data source. Columns: `id\tdescription`.

```
ls	List directory contents
ls-a	List all entries including hidden files
```

### UI strings

`strings-{lang}.txt` — maps string IDs to localized UI text.

```
menu_play	Play
menu_quit	Quit
```

Supported language codes: `eng`, `fi`.

## Data sources

| Source | Flag | File | Description |
|---|---|---|---|
| Core | `SOURCE_CORE` (1) | `core.txt` | Common Linux commands |
| Hotkeys | `SOURCE_HOTKEYS` (2) | `hotkeys.txt` | Terminal keyboard shortcuts |
| Builtins | `SOURCE_BUILTINS` (4) | `builtins.txt` | Shell built-in commands |
| Redirection | `SOURCE_REDIR` (8) | `redir.txt` | Pipes and redirection operators |
| Debian | `SOURCE_DEBIAN` (16) | `debian.txt` | Debian/Ubuntu-specific commands |
| RHEL | `SOURCE_RHEL` (32) | `rhel.txt` | RHEL/Fedora/CentOS-specific commands |
| Arch | `SOURCE_ARCH` (64) | `arch.txt` | Arch Linux-specific commands |
| macOS | `SOURCE_MACOS` (128) | `macos.txt` | macOS-specific commands |

## Categories

| ID | Category |
|---|---|
| `file` | File operations |
| `text` | Text processing |
| `net` | Networking |
| `proc` | Process management |
| `sys` | System information |
| `perm` | Users & permissions |
| `pkg` | Package management |
| `dev` | Development tools |
| `shell` | Shell basics |
| `hotkeys` | Terminal hotkeys |
| `redir` | Redirection & pipes |

## Difficulty system

The game tracks 20 difficulty levels. As you answer questions:

- The current maximum difficulty starts at 1 and grows as you demonstrate proficiency
- To unlock the next difficulty level, you need **80% accuracy** across the current level and at least **3 mastered commands** at that level
- Mastery requires **5 consecutive correct answers** on a command
- Questions are drawn from your current maximum difficulty and below

## Progress

The initial version was completely vibecoded with OpenCode Zen free models.

- Current streak of consecutive correct answers
- Total number of first-attempt answers
- Mastery status (5 consecutive = mastered)

## Contributing

### Adding a new command

1. Open the appropriate `data/{source}.txt` master file
2. Add a new line with a unique `id`, the `command`, a `category` ID, and a `difficulty` level
3. Add a description to `data/{source}-eng.txt` (and `data/{source}-fi.txt` for Finnish)
4. Rebuild and test

The `id` must be a stable slug: lowercase, hyphens for special characters (e.g. `ls-la-color`, `redirect-err-out`).

### Adding a new translation language

1. Create `data/{source}-{lang}.txt` for each of the 8 sources, translating the `description` field
2. Create `data/strings-{lang}.txt` with UI string translations (see `str.h` for the full list of string IDs)
3. Add the language enum to `settings.h` (e.g. `LANG_GERMAN = 2`)
4. Add the suffix to the `suffix[]` array in `main.c` (e.g. `-de.txt`)
5. Add the language option in `settings.c` (the `settings_menu` function)
6. Update `str.c` if new string IDs are needed

### Adding a new data source

1. Create `data/{name}.txt` with master entries
2. Create `data/{name}-eng.txt` and `data/{name}-fi.txt` with translations
3. Add a `SOURCE_{NAME}` flag in `data.h` (next power of two)
4. Add the source ID to `source_ids[]` in `data.c`
5. Add string IDs for the source name in `str.h` and `data/strings-*.txt`
6. Wire it up in `main.c` (add to `source_flags[]`, `master_name()`, and `lang_name()`)
7. Rebuild and test

