#ifndef CROCHET_API
#define CROCHET_API

#include <stdint.h>
#include <stdlib.h>
#include <pprint/pprint.h>

/*
 * Every type of chart object that can be drawn.
 */
typedef enum { CHART_OBJECT_TEXT = 0, CHART_OBJECT_LINE = 1 } chart_object_t;

typedef enum {
  WHITE_MARUBUZU = 0,
  BLACK_MARUBUZU = 1,
  LONG_LEGGED_DRAGON_FLY_DOJI = 2,
  DRAGON_FLY_DOJI = 3,
  GRAVESTONE_DOJI = 4,
  FOUR_PRICE_DOJI = 5,
  HANGING_MAN = 6,
  SHOOTING_STAR = 7,
  SPINNING_TOP = 8,
  SUPPORT_LINE = 9,
  RESISTANCE_LINE = 10
} analysis_shortname_t;

/*
 * Represents a generic object that gets displayed on the chart
 */
struct chart_object {
  chart_object_t object_type;
  analysis_shortname_t shortname;
  void *value;
  struct chart_object *next;
};

struct chart_object_t_text {
  char TEXT;
};

struct chart_object_t_line {
  size_t start;
  uint32_t start_price;

  size_t end;
  uint32_t end_price;
};

/*
 * Represents a candle in the chart
 */
struct candle {
  /*
   *    |   <---- high
   *    |
   *  ----- <---- open or close
   *  |   |
   *  |   |
   *  |   |
   *  |   |
   *  ----- <---- close or open
   *    |
   *    |   <---- low
   *
   */
  uint32_t open;
  uint32_t high;
  uint32_t low;
  uint32_t close;
  uint32_t volume;

  struct chart_object *analysis_list;
};

#define CHART_MINUTES_IN_WEEK 10080
/*
 * Represents a chart and all the elements needed for analysis to draw
 * correctly and informative information on the chart
 */
struct chart {

  /*
   * Number of candles used in the buffer
   */
  size_t num_candles;

  /*
   * The candle index of the current candle
   */
  size_t cur_candle_idx;

  /*
   * A list of candles by default the 1 minute
   */
  struct candle *candles;

  /*
   * A list of chart objects that were added to the chart
   */
  struct chart_object *chart_objects;
};

/*
 * Generic definition for analysis
 *
 * @param cnds A list of candles
 * @param indx The last candle + 1. Loops should be performed on the interval
 * [0, indx)
 */
typedef void (*analysis_func)(struct candle *cnds, size_t indx);

void
chart_create_object_line(
    struct candle *cnd, size_t start_idx, size_t start_price,
    size_t end_idx, size_t end_price, analysis_shortname_t shortname);

void
chart_create_object_text(
    struct candle *cnd, char c, analysis_shortname_t shortname);

#endif