//
// Created by Aaron Gill-Braun on 2021-07-02.
//

#include "P3Panel.h"

#define FONT_WIDTH 6
#define FONT_HEIGHT 8

#define index(r, c) ((r) * m_cols + (c))

extern unsigned char const panel_font_6x8[128][8];


P3Panel::P3Panel(uint8_t rows, uint8_t cols) {
  m_rows = rows;
  m_cols = cols;
  m_front = new uint32_t[rows * cols];
  m_back = new uint32_t[rows * cols];
  m_dirty = true;

  int num = spin_lock_claim_unused(true);
  m_lock = spin_lock_init(num);
  m_flags = 0;
  m_lock_count = 0;

  memset(m_front, 0, sizeof(uint32_t) * rows * cols);
  memset(m_back, 0, sizeof(uint32_t) * rows * cols);

  gpio_init_mask(PIN_MASK);
  gpio_set_dir_out_masked(PIN_MASK);
}

P3Panel::~P3Panel() {
  delete m_front;
  delete m_back;
}


void P3Panel::setPixel(uint8_t x, uint8_t y, uint32_t rgb) {
  if (x >= m_cols || y >= m_rows)
    return;

  beginDraw();
  m_back[index(y, x)] = rgb;
  endDraw();
}

void P3Panel::fillRow(uint8_t y, uint32_t rgb) {
  if (y >= m_rows)
    return;

  beginDraw();
  for (int i = 0; i < m_cols; i++) {
    m_back[index(y, i)] = rgb;
  }
  endDraw();
}

void P3Panel::fillColumn(uint8_t x, uint32_t rgb) {
  if (x >= m_rows)
    return;

  beginDraw();
  for (int i = 0; i < m_rows; i++) {
    m_back[index(i, x)] = rgb;
  }
  endDraw();
}

void P3Panel::drawString(const char *str, uint8_t x, uint8_t y, uint32_t rgb, bool large) {
  size_t len = strlen(str);
  uint32_t offset = 0;
  uint32_t mult = large ? 2 : 1;
  for (size_t i = 0; i < len; i++) {
    if (str[i] == '\n') {
      y += FONT_HEIGHT * mult;
      x = 0;
      offset = 0;

      if (y >= m_rows)
        return;
      else
        continue;
    }

    if (large) {
      drawBigCharacter(str[i], x + offset, y, rgb);
    } else {
      drawCharacter(str[i], x + offset, y, rgb);
    }
    offset += FONT_WIDTH * mult;
  }
}

void P3Panel::clear() {
  beginDraw();
  memset(m_back, 0, sizeof(uint32_t) * m_rows * m_cols);
  endDraw();
}


void P3Panel::beginDraw() {
  if (m_lock_count == 0)
    m_flags = spin_lock_blocking(m_lock);
  m_lock_count++;
}

void P3Panel::endDraw() {
  m_dirty = true;
  m_lock_count--;
  if (m_lock_count == 0)
    spin_unlock(m_lock, m_flags);
}


void P3Panel::update() {
  if (m_dirty)
    swapBuffers();

  for (int i = 0; i < m_rows / 2; i++) {
    uint32_t *a = m_front + index(i, 0);
    uint32_t *b = m_front + index(i + (m_rows / 2), 0);

    selectLine(i);
    gpio_put(OE, HIGH);
    for (int j = 0; j < m_cols; j++) {
      uint32_t p1 = a[j];
      uint32_t p2 = b[j];

      gpio_put(RF, GET_R(p1) != 0);
      gpio_put(RS, GET_R(p2) != 0);
      gpio_put(GF, GET_G(p1) != 0);
      gpio_put(GS, GET_G(p2) != 0);
      gpio_put(BF, GET_B(p1) != 0);
      gpio_put(BS, GET_B(p2) != 0);

      clock();
    }
    latch();
    gpio_put(OE, LOW);
    busy_wait_us(500);
  }
}

//

void P3Panel::drawCharacter(char c, uint8_t x, uint8_t y, uint32_t rgb) {
  if (x >= m_cols || y >= m_rows)
    return;

  const uint8_t *font = panel_font_6x8[(uint8_t) c];

  beginDraw();
  for (int i = 0; i < FONT_HEIGHT; i++) {
    if (i + y >= m_rows)
      break;

    uint8_t row = font[i];
    for (int b = 0; b < FONT_WIDTH; b++) {
      if (b + x >= m_cols)
        break;

      uint8_t v = row & (1 << (FONT_WIDTH - b));
      if (v) {
        m_back[index(i + y, b + x)] = rgb;
      }
    }
  }
  endDraw();
}

void P3Panel::drawBigCharacter(char c, uint8_t x, uint8_t y, uint32_t rgb) {
  if (x >= m_cols || y >= m_rows)
    return;

  const uint8_t *font = panel_font_6x8[(uint8_t) c];
  uint32_t xOffset = 0;
  uint32_t yOffset = 0;

  // this scales the default font up 2x
  beginDraw();
  for (int i = 0; i < FONT_HEIGHT; i++) {
    if (i + yOffset + y + 1 >= m_rows)
      break;

    uint8_t row = font[i];

    for (int b = 0; b < FONT_WIDTH; b++) {
      if (b + xOffset + x + 1 >= m_cols)
        break;

      uint8_t v = row & (1 << (FONT_WIDTH - b));
      if (v) {
        m_back[index(i + y + yOffset, b + x + xOffset)] = rgb;
        m_back[index(i + yOffset + y + 1, b + xOffset + x)] = rgb;
        m_back[index(i + yOffset + y, b + xOffset + x + 1)] = rgb;
        m_back[index(i + yOffset + y + 1, b + xOffset + x + 1)] = rgb;
      }
      xOffset += 1;
    }

    xOffset = 0;
    yOffset += 1;
  }
  endDraw();
}

//

void P3Panel::swapBuffers() {
  uint32_t saved;
  if (spin_trylock(m_lock, &saved)) {
    uint32_t *temp = m_front;
    m_front = m_back;
    m_back = temp;
    m_dirty = false;
    spin_unlock(m_lock, saved);
  }
}

void P3Panel::selectLine(uint8_t line) {
  if (line >= m_rows / 2)
    return;

  if (line == 0) {
    gpio_put(RA, HIGH);
    gpio_put(RB, HIGH);
    gpio_put(RC, HIGH);
    gpio_put(RD, HIGH);

    gpio_put(RA, LOW);
    gpio_put(RB, LOW);
    gpio_put(RC, LOW);
    gpio_put(RD, LOW);
    return;
  }

  gpio_put(RA, line & 0b0001);
  gpio_put(RB, line & 0b0010);
  gpio_put(RC, line & 0b0100);
  gpio_put(RD, line & 0b1000);
}

void P3Panel::clock() {
  gpio_put(CLK, HIGH);
  gpio_put(CLK, LOW);
}

void P3Panel::latch() {
  gpio_put(LAT, HIGH);
  gpio_put(LAT, LOW);
}




