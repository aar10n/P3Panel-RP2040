#include <P3Panel.h>
#include <clocks.h>
#include <multicore.h>

P3Panel panel(32, 64);
uint32_t row = 0;
uint32_t col = 0;


void draw_loop() {
  while (true) {
    panel.update();
  }
}

void setup() {
  clock_configure(clk_sys,
                  CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                  CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                  125 * MHZ,
                  60 * MHZ);

  sleep_ms(1000);
  multicore_launch_core1(draw_loop);
  sleep_ms(1000);
}

void loop() {
  panel.beginDraw();
  panel.clear();
  panel.setPixel(col, row, 0x0000FF);
  panel.endDraw();

  if (row % 2 == 0) {
    col++;
    if (col >= 64) {
      col = 63;
      row++;
    }
  } else {
    col--;
    if (col <= 0) {
      col = 0;
      row++;
    }
  }

  if (row >= 32) {
    row = 0;
    col = 0;
  }
  sleep_ms(50);
}
