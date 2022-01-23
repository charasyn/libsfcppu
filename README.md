libsfcppu
=========

This library renders SFC/SNES graphical data into a framebuffer.

This library exposes two C API functions:
- `bool libsfcppu_init()`
  - Call this to initialize the library.
- `uint16 const * libsfcppu_drawFrame( LibSFCPPU::SnesFrameData const * ppuState )`
  - Call this to render the data specified in `ppuState`.
  - The pointer returned points to a 512x480 buffer containing the pixel data of the rendered frame.
  - Output pixel format: `0bbb bbgg gggr rrrr`
  - See `data-interop.hpp` for the exact format of `ppuState`.

You should hopefully be able to call `make` to build a dynamic object containing the library.
This library has only been tested on Windows MinGW-w64.

License
-------

See `LICENSE.txt`. All code not under the `nall/` subdirectory is covered by bsnes' licensing terms.

Links
-----

  - [bsnes official website](https://bsnes.dev)
  - [bsnes official git repository](https://github.com/bsnes-emu/bsnes)
