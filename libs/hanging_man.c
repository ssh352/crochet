#include <lib.h>

void
analysis_check_hanging_man(struct candle *cnds, size_t indx)
{
  uint32_t x, y, z;

  if (cnds[indx - 1].open < cnds[indx - 1].close) {
    /* calculate x,y,z for candles who was bullish */

    /* close > open therefore high - close > 0 */
    x = cnds[indx - 1].high - cnds[indx - 1].close;
    y = cnds[indx - 1].close - cnds[indx - 1].open;
    z = cnds[indx - 1].open - cnds[indx - 1].low;

    if (x < z && z > 2 * x && y < z) {
      chart_create_object_text(&(cnds[indx - 1]), 'H', __func__);
    }
  } else if (cnds[indx - 1].close < cnds[indx - 1].open) {
    /* calculate x,y,z for candles who was bullish */

    /* close > open therefore high - close > 0 */
    x = cnds[indx - 1].high - cnds[indx - 1].open;
    y = cnds[indx - 1].open - cnds[indx - 1].close;
    z = cnds[indx - 1].close - cnds[indx - 1].low;

    if (x < z && z > 2 * x && y < z) {
      chart_create_object_text(&(cnds[indx - 1]), 'H', __func__);
    }
  }
}

struct vtable exports = {
  &analysis_check_hanging_man
};
