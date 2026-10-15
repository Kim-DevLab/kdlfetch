# kdlfetch

`kdlfetch` is a small Linux system-information program written in C, inspired by
fastfetch and neofetch. It prints a compact system summary next to a simple
ASCII logo and has no external runtime dependencies.

## Requirements

- Linux
- GCC with C23 support
- GNU Make

## Build and run

```sh
make
make run
```

Build an optimized executable with `make release`. Run the parser tests with
`make test`, or remove generated executables with `make clean`.

The executable is written to `bin/kdlfetch`. If a system-information source is
unavailable, `kdlfetch` reports a warning, prints `Unavailable` for that value,
and exits with a non-zero status.

## Information shown

- Linux distribution name from `/etc/os-release`
- Kernel release and hostname
- Uptime
- CPU model
- Used and total memory
- Used and total space on the root filesystem

The source code is in `src/`, tests are in `tests/`, and project notes are in
`docs/`.
