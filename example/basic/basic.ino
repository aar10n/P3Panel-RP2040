#include <P3Panel.h>
#include <clocks.h>

P3Panel panel(32, 64);

void setup() {
  clock_configure(clk_sys,
                  CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                  CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                  125 * MHZ,
                  60 * MHZ);

  panel.drawString("Hello\nworld", 0, 0, 0x000000FF);
}

void loop() {
  panel.update();
}
