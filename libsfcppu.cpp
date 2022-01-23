#include <nall/platform.hpp>
#include <nall/random.hpp>
using namespace nall;

#include <emulator/types.hpp>
#include <emulator/memory/readable.hpp>
#include <emulator/memory/writable.hpp>
#include <emulator/random.hpp>

namespace LibSFCPPU {
  #include "data-interop.hpp"
  struct PPUWrapper {
    struct Platform {
      uint16 const * gfxData;
      uint gfxPitch;
      uint gfxWidth;
      uint gfxHeight;
      uint gfxScale;
      auto videoFrame(const uint16* data, uint pitch, uint width, uint height, uint scale) -> void {
        gfxData = data;
        gfxPitch = pitch;
        gfxWidth = width;
        gfxHeight = height;
        gfxScale = scale;
      }
    };
    Platform * platform;
    Platform platformObj;
    SnesFrameData const * currentFrameData;
    int nextHdmaWriteToProcess;

    PPUWrapper() : platform(&platformObj), currentFrameData(nullptr), nextHdmaWriteToProcess(0) {}

    bool init();
    void drawFrame(SnesFrameData const *);
    void scanline();
  } ppuwrapper;
}

#define ppuwrapper LibSFCPPU::ppuwrapper

extern "C" {
  bool libsfcppu_init() {
    return ppuwrapper.init();
  }
  uint16 const * libsfcppu_drawFrame(LibSFCPPU::SnesFrameData const * ppuState) {
    ppuwrapper.drawFrame(ppuState);
    return ppuwrapper.platformObj.gfxData;
  }
}

struct cothread_t {};

namespace SuperFamicom {
  #define platform ppuwrapper.platform
  using Random = Emulator::Random;
  extern Random random;

  struct Scheduler {
    enum class Mode : uint { Run, Synchronize } mode;
    enum class Event : uint { Frame, Synchronized, Desynchronized } event;

    bool desynchronized = false;

    auto enter() -> void {
      // host = co_active();
      // co_switch(active);
    }

    auto leave(Event event_) -> void {
      // event = event_;
      // active = co_active();
      // co_switch(host);
    }

    auto resume(cothread_t thread) -> void {
      if(mode == Mode::Synchronize) desynchronized = true;
      // co_switch(thread);
    }

    inline auto synchronizing() const -> bool {
      return mode == Mode::Synchronize;
    }

    inline auto synchronize() -> void {
      if(mode == Mode::Synchronize) {
        if(desynchronized) {
          desynchronized = false;
          leave(Event::Desynchronized);
        } else {
          leave(Event::Synchronized);
        }
      }
    }

    inline auto desynchronize() -> void {
      desynchronized = true;
    }
  };
  extern Scheduler scheduler;

  struct Thread {
    enum : uint { Size = 4 * 1024 * sizeof(void*) };

    auto create(auto (*entrypoint)() -> void, uint frequency_) -> void {
      frequency = frequency_;
      clock = 0;
    }

    auto active() const -> bool {
      return true;
    }

    auto serialize(serializer& s) -> void {
      s.integer(frequency);
      s.integer(clock);
    }

      uint32_t frequency = 0;
       int64_t clock = 0;
  };
  
  namespace Region {
    bool NTSC() { return true; }
    bool PAL() { return false; }
  }
  struct CPU {
    uint8 pio() { return 0; }
    void synchronizePPU() {}
    void scanline() { ppuwrapper.scanline(); }
    cothread_t thread;
  };
  CPU cpu;
  Scheduler scheduler;
  Random random;

  #include "counter/counter.hpp"
  #include "ppu.hpp"
  #include "counter/counter-inline.hpp"
}

// Include bsnes' PPU code
#include "ppu.cpp"

#define ppu SuperFamicom::ppu

namespace LibSFCPPU {
  void ppuStepUntilVBlank() {
    while(ppu.vcounter() < ppu.vdisp()) ppu.main();
  }
  void ppuStepOneFrame() {
    while(ppu.vcounter() >= ppu.vdisp()) ppu.main();
    // Frame has begun
    ppuStepUntilVBlank();
  }
  bool PPUWrapper::init() {
    SuperFamicom::random.entropy(Emulator::Random::Entropy::Low);
    ppu.power(0);
    ppu.refresh();
    if (
      platformObj.gfxWidth != 512 ||
      platformObj.gfxHeight != 480 ||
      platformObj.gfxPitch != 512 * 2 ||
      platformObj.gfxScale != 1
    ) {
      return false;
    }

    ppuStepUntilVBlank();
    return true;
  }
  void PPUWrapper::drawFrame(SnesFrameData const * const d) {
    // Reset our state
    currentFrameData = d;
    nextHdmaWriteToProcess = 0;

    // Initialise PPU state
    ppu.writeIO(0x2100, d->INIDISP);
    ppu.writeIO(0x2101, d->OBSEL);
    // Necessary, in case the OAM priority feature is used.
    ppu.writeIO(0x2102, d->OAMADDR);
    ppu.writeIO(0x2103, d->OAMADDR >> 8);
    ppu.writeIO(0x2105, d->BGMODE);
    ppu.writeIO(0x2106, d->MOSAIC);
    ppu.writeIO(0x2107, d->BG1SC);
    ppu.writeIO(0x2108, d->BG2SC);
    ppu.writeIO(0x2109, d->BG3SC);
    ppu.writeIO(0x210A, d->BG4SC);
    ppu.writeIO(0x210B, d->BG12NBA);
    ppu.writeIO(0x210C, d->BG34NBA);
    ppu.writeIO(0x210D, d->BG1HOFS); ppu.writeIO(0x210D, d->BG1HOFS >> 8);
    ppu.writeIO(0x210E, d->BG1VOFS); ppu.writeIO(0x210E, d->BG1VOFS >> 8);
    ppu.writeIO(0x210F, d->BG2HOFS); ppu.writeIO(0x210F, d->BG2HOFS >> 8);
    ppu.writeIO(0x2110, d->BG2VOFS); ppu.writeIO(0x2110, d->BG2VOFS >> 8);
    ppu.writeIO(0x2111, d->BG3HOFS); ppu.writeIO(0x2111, d->BG3HOFS >> 8);
    ppu.writeIO(0x2112, d->BG3VOFS); ppu.writeIO(0x2112, d->BG3VOFS >> 8);
    ppu.writeIO(0x2113, d->BG4HOFS); ppu.writeIO(0x2113, d->BG4HOFS >> 8);
    ppu.writeIO(0x2114, d->BG4VOFS); ppu.writeIO(0x2114, d->BG4VOFS >> 8);
    ppu.writeIO(0x211A, d->M7SEL);
    ppu.writeIO(0x211B, d->M7A); ppu.writeIO(0x211B, d->M7A >> 8);
    ppu.writeIO(0x211C, d->M7B); ppu.writeIO(0x211C, d->M7B >> 8);
    ppu.writeIO(0x211D, d->M7C); ppu.writeIO(0x211D, d->M7C >> 8);
    ppu.writeIO(0x211E, d->M7D); ppu.writeIO(0x211E, d->M7D >> 8);
    ppu.writeIO(0x211F, d->M7X); ppu.writeIO(0x211F, d->M7X >> 8);
    ppu.writeIO(0x2120, d->M7Y); ppu.writeIO(0x2120, d->M7Y >> 8);
    ppu.writeIO(0x2123, d->W12SEL);
    ppu.writeIO(0x2124, d->W34SEL);
    ppu.writeIO(0x2125, d->WOBJSEL);
    ppu.writeIO(0x2126, d->WH0);
    ppu.writeIO(0x2127, d->WH1);
    ppu.writeIO(0x2128, d->WH2);
    ppu.writeIO(0x2129, d->WH3);
    ppu.writeIO(0x212A, d->WBGLOG);
    ppu.writeIO(0x212B, d->WOBJLOG);
    ppu.writeIO(0x212C, d->TM);
    ppu.writeIO(0x212D, d->TS);
    ppu.writeIO(0x212E, d->TMW);
    ppu.writeIO(0x212F, d->TSW);
    ppu.writeIO(0x2130, d->CGWSEL);
    ppu.writeIO(0x2131, d->CGADSUB);
    ppu.writeIO(0x2132, 0x20 | d->FIXED_COLOR_DATA_R & 0x1f);
    ppu.writeIO(0x2132, 0x40 | d->FIXED_COLOR_DATA_G & 0x1f);
    ppu.writeIO(0x2132, 0x80 | d->FIXED_COLOR_DATA_B & 0x1f);
    ppu.writeIO(0x2133, d->SETINI);

    memcpy(ppu.vram.data, d->vram, sizeof(d->vram));
    for(int i = 0; i < 256; i++) ppu.screen.cgram[i] = d->cgram[i];
    for(int i = 0, addr = 0; i < 128; i++, addr += 4) {
      ppu.obj.oam.write(addr + 0, d->oam1[i].xCoord);
      ppu.obj.oam.write(addr + 1, d->oam1[i].yCoord);
      ppu.obj.oam.write(addr + 2, d->oam1[i].startingTile);
      ppu.obj.oam.write(addr + 3, d->oam1[i].flags);
    }
    for(int i = 0; i < 32; i++) ppu.obj.oam.write(i + 512, d->oam2[i]);

    // Step PPU until frame is rendered
    ppuStepOneFrame();

    // Reset our pointer to the frame data so we can't reuse it outside
    // of the scope of this function.
    currentFrameData = nullptr;
  }
  // Called by ppu.main() each scanline - updates 
  void PPUWrapper::scanline() {
    const auto d = currentFrameData;
    // If we aren't properly initialized, return
    if (!d) return;
    // If we have no HDMA accesses to process, return
    if (nextHdmaWriteToProcess >= d->numHdmaWrites) return;

    const auto vcounter = ppu.vcounter();
    if (vcounter >= ppu.vdisp()) return;

    int i = nextHdmaWriteToProcess;
    assert(d->hdmaData[i].vcounter >= vcounter);
    for (; i < d->numHdmaWrites && d->hdmaData[i].vcounter == vcounter; i++) {
      ppu.writeIO(d->hdmaData[i].addr | 0x2100, d->hdmaData[i].value);
    }
    nextHdmaWriteToProcess = i;
  }
}

