# `winscap` [![][1]][2]

Windows raw sound output capture tool.

## Usage

You tell `winscap` how many channels and bits to capture at what sampling rate:

    winscap <channels> <sample rate> <bits per sample>

`winscap` will output a raw PCM stream (signed little-endian) to stdout.

Note that `winscap` uses your audio device in shared mode, so your capture settings
must match the Windows output device. By default, this is 16-bit stereo at 48000 Hz.
If your capture settings do not match, `winscap` will fail to start.

## Binaries

The latest stable binaries are available on [GitHub releases][3] ([winscap.exe][4]).

Latest bleeding edge binaries are available as artifacts on [GitHub Actions][2].

## Building

You need a new enough Visual C++ toolchain. To build, run

    nmake

## Background

I created this tool as a lightweight approach to run [Cava][5] on Windows Subsystem
for Linux (WSL) while using sound output from Windows.

To use with Cava, configure Cava to read from a named pipe inside WSL (we'll use
`/tmp/cava.fifo` in this example) with the following configuration:

```ini
[input]
method = fifo
source = /tmp/cava.fifo
sample_rate = 48000
```

Replace `48000` with whatever sampling rate you use with `winscap`.

Then, run `winscap` as follows:

```sh
$ mkfifo /tmp/cava.fifo
$ /mnt/c/path/to/winscap.exe 2 48000 16 > /tmp/cava.fifo
```

Again, replace the arguments as appropriate.

  [1]: https://github.com/quantum5/winscap/workflows/build/badge.svg
  [2]: https://github.com/quantum5/winscap/actions
  [3]: https://github.com/quantum5/winscap/releases
  [4]: https://github.com/quantum5/winscap/releases/latest/download/winscap.exe
  [5]: https://github.com/karlstav/cava
