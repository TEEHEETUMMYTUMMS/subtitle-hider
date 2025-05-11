# Subtitle Hider

![Demo](./media/subhide.gif)

A frameless, draggable, and resizable transparent window for X11 designed to
hide subtitles with a blur effect so that words are unreadable but content can
still be seen

blur/transparency affects requires picom or other compositor
resize by draging the corners

## Dependencies

### Required Libraries.

- `libX11` - Core X11 window handling
- `libXext` - For window hints (`_MOTIF_WM_HINTS`)
- `libXrender` - For transparency visuals

### Required for Transparency

- **`picom`** - Lightweight X11 compositor that handles transparency and blur

## Installation
### Install Dependencies
```
sudo apt update
sudo apt install build-essential libx11-dev libxext-dev libxrender-dev picom stow
```
### Clone Repo
```
git clone https://github.com/TEEHEETUMMYTUMMS/subtitle-hider.git
```
## Build & Run

### Make bin dir:

```
cd subtitle-hider
mkdir -p bin/.local/bin
```
### Compile with `gcc`:

```
gcc ./subtitle-hider -o ./bin/.local/bin/subtitle-hider -lX11
```
### Stow and then Run with:

```
stow bin -t $HOME
```
```
subtitle-hider
```
## Setting up `picom`

Here is an example `picom.conf` that applies full transparency with a blur to
the `SubtitleHider` windows:

```bash
backend = "glx";

blur-method = "dual_kawase";
blur-strength = 5;

blur-background-exclude = [
    "!(class_g = 'SubtitleHider')",
]

opacity-rule = [
    "0:class_g = 'SubtitleHider'",
]
```
Save this to:

```
~/.config/picom/picom.conf
```

