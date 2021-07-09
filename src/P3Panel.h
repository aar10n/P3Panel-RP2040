//
// Created by Aaron Gill-Braun on 2021-07-02.
//

#ifndef LIB_LEDPANEL_SRC_PANEL_H
#define LIB_LEDPANEL_SRC_PANEL_H

#include <Arduino.h>
#include <Pins.h>
#include <hardware/sync.h>

#define RF D2   // red first byte
#define GF D3   // green first byte
#define BF D4   // blue first byte
#define RS D5   // red second byte
#define GS D6   // green second byte
#define BS D7   // blue second byte
#define CLK D8  // clock signal
#define OE D9   // output enable
#define LAT D10 // data latch
#define RA A0   // register selector a
#define RB A1   // register selector b
#define RC A2   // register selector c
#define RD A3   // register selector d

#define _M(b) (1 << (b))
#define _MV(b, v) ((v) << (b))
#define PIN_MASK (_M(RF) | _M (GF) | _M(BF) | _M(RS) | _M(GS) | _M(BS) | \
                  _M(CLK) | _M(OE) | _M(RA) | _M(RB) | _M(RC) | _M(RD) | _M(LAT))
#define LINE_MASK (_M(RA) | _M(RB) | _M(RC) | _M(RD))
#define COLOR_MASK (_M(RF) | _M(RS) | _M(GF) | _M(GS) | _M(BF) | _M(BS))

#define LNSEL_MASK(a, b, c, d) (_MV(RA, a) | _MV(RB, b) | _MV(RC, c) | _MV(RD, d))

#define GET_R(v) ((v) & 0x000000FF)
#define GET_G(v) ((v) & 0x0000FF00)
#define GET_B(v) ((v) & 0x00FF0000)

inline static bool spin_trylock(spin_lock_t *lock, uint32_t *saved_irq) {
  check_hw_size(spin_lock_t, 4);
  if (saved_irq == nullptr)
    return false;

  uint32_t save = save_and_disable_interrupts();
  if (__builtin_expect(!*lock, 0)) {
    restore_interrupts(save);
    return false;
  }
  __mem_fence_acquire();
  *saved_irq = save;
  return true;
}


class P3Panel {
private:
  uint8_t m_rows;
  uint8_t m_cols;
  uint32_t *m_front;
  uint32_t *m_back;
  bool m_dirty;
  spin_lock_t *m_lock;
  uint32_t m_flags;
  uint32_t m_lock_count;

public:
  explicit P3Panel(uint8_t rows, uint8_t cols);
  virtual ~P3Panel();

  /**
   * Sets the pixel at the specified row and column to the given value.
   * @param x column
   * @param y row
   * @param rgb color
   */
  void setPixel(uint8_t x, uint8_t y, uint32_t rgb);

  /**
   * Fills the specified row with the given color.
   * @param y row
   * @param rgb color
   */
  void fillRow(uint8_t y, uint32_t rgb);

  /**
   * Fills the specified column with the given color.
   * @param x column
   * @param rgb color
   */
  void fillColumn(uint8_t x, uint32_t rgb);

  /**
   * Draws a string with the first character's origin at the specified row and column.
   * @param str the string to draw
   * @param x row
   * @param y column
   * @param rgb color
   * @param large use large font
   */
  void drawString(const char *str, uint8_t x, uint8_t y, uint32_t rgb, bool large = false);

  /** Clears the entire panel. */
  void clear();

  /** Aquires the draw buffer. */
  void beginDraw();
  /** Releases the draw buffer. */
  void endDraw();

  /** Updates the panel. */
  void update();

private:
  void drawCharacter(char c, uint8_t x, uint8_t y, uint32_t rgb);
  void drawBigCharacter(char c, uint8_t x, uint8_t y, uint32_t rgb);
  void swapBuffers();
  void selectLine(uint8_t line);
  static void clock();
  static void latch();
};


#endif
