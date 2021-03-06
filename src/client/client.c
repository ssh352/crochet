#include "client.h"

/* Display to display to */
static Display *dis;

/* Screen to display to */
static int screen;

/* Window handle for this window */
static Window win;

/* Graphics context */
static GC gc;

/* hold command buffer */
char command[256] = {0};
size_t command_idx = 0;

/* the font to be used */
XFontStruct *font_info;

/* the security that is being display */
struct security *sec = NULL;

/* the double buffer */
Pixmap double_buffer = 0;

/* last recorded window size */
int width = 0;
int height = 0;

/* last recorded x and y location of the mouse */
int mouse_x;
int mouse_y;

/* color for data that is bullish */
XColor upward;

/* color for data that is bearish */
XColor downward;

/* color for the background color */
XColor background;

/* color for the foreground */
XColor foreground;

/* color orange */
XColor orange;

/* mutex lock the draw functions for single threaded drawing */
pthread_mutex_t draw_mutex;

bool finished_displaying = true;

static void
_client_init()
{

  /* enable draw updates */
  finished_displaying = false;

  /* Open up the display set by the DISPLAY environmental variable */
  dis = XOpenDisplay(NULL);

  /* get the default screen "monitor" of this display */
  screen = DefaultScreen(dis);

  /* get the colors black and white from the screen */
  unsigned long black;
  unsigned long white;
  black = BlackPixel(dis, screen);
  white = WhitePixel(dis, screen);

  /* create the window */
  win = XCreateSimpleWindow(dis, RootWindow(dis, screen), 0, 0, 1024, 720, 0,
                            black, black);

  /* set window class and name for compatibility with i3 */
  XClassHint xch;
  xch.res_class = "dialog";
  xch.res_name = "riski";
  XSetClassHint(dis, win, &xch);
  XSetStandardProperties(dis, win, "Feed", NULL, None, NULL, 0, NULL);

  /* create a graphics context to draw on */
  gc = XCreateGC(dis, win, 0, 0);

  /* set the default foreground and background colors */
  XSetBackground(dis, gc, white);
  XSetForeground(dis, gc, black);

  XSetFillStyle(dis, gc, FillSolid);

  /* setup input for exposure button and keypress */
  XSelectInput(dis, win, ExposureMask | ButtonPressMask | KeyPressMask);

  font_info = XLoadQueryFont(dis, "fixed");
  XSetFont(dis, gc, font_info->fid);

  Colormap colormap = XDefaultColormap(dis, screen);

  char green[] = "#628E36";
  XParseColor(dis, colormap, green, &upward);
  XAllocColor(dis, colormap, &upward);

  char red[] = "#f44336";
  XParseColor(dis, colormap, red, &downward);
  XAllocColor(dis, colormap, &downward);

  char dark[] = "#1F1B24";
  XParseColor(dis, colormap, dark, &background);
  XAllocColor(dis, colormap, &background);

  char fg[] = "#F5DEB3";
  XParseColor(dis, colormap, fg, &foreground);
  XAllocColor(dis, colormap, &foreground);

  char org[] = "#FF7F00";
  XParseColor(dis, colormap, org, &orange);
  XAllocColor(dis, colormap, &orange);

  /* clear the window and bring it to the top */
  XClearWindow(dis, win);
  XMapRaised(dis, win);
}

static int
_process_keypress(XEvent event)
{

  KeySym key; /* a dealie-bob to handle KeyPress Events */

  if (XLookupString(&event.xkey, &(command[command_idx]), 255, &key, 0) == 1)
  {
    if (command[command_idx] == 0x00)
    {
      command[command_idx] = '\x0';
      return false;
    }

    /* user pressed back space */
    if (command[command_idx] == 0x08)
    {
      command[command_idx] = '\x0';
      command_idx -= 1;
      return false;
    }
    /* user pressed enter */
    else if (command[command_idx] == 0x0d)
    {
      command[command_idx] = '\x0';
      pprint_info("command: %s", command);
      sec = exchange_get(command);
      if (!sec)
      {
      }
      command_idx = 0;
      return false;
    }
    else
    {
      command_idx += 1;
    }
  }
  return false;
}

static void
_redraw()
{
  pthread_mutex_lock(&draw_mutex);
  static const int pow10[7] = {1.0,     10.0,     100.0,    1000.0,
                               10000.0, 100000.0, 1000000.0};

  XWindowAttributes xwa;
  XGetWindowAttributes(dis, win, &xwa);

  if (xwa.width < 480 || xwa.height < 240)
  {
    pthread_mutex_unlock(&draw_mutex);
    return;
  }

  if (xwa.width != width || xwa.height != height)
  {
    if (double_buffer == 0)
    {
      double_buffer = XCreatePixmap(dis, win, xwa.width, xwa.height, xwa.depth);
    }
    else
    {
      XFreePixmap(dis, double_buffer);
      double_buffer = XCreatePixmap(dis, win, xwa.width, xwa.height, xwa.depth);
    }
  }

  width = xwa.width;
  height = xwa.height;

  /* set the color to white and a solid fill for drawing */
  XSetForeground(dis, gc, background.pixel);
  XSetFillStyle(dis, gc, FillSolid);
  XFillRectangle(dis, double_buffer, gc, 0, 0, xwa.width, xwa.height);

  /* set the color to white and a solid fill for drawing */
  XSetForeground(dis, gc, foreground.pixel);
  XSetFillStyle(dis, gc, FillSolid);

  int font_height = font_info->ascent + font_info->descent;
  int candle_width = font_info->per_char->width + 3;

  XSetForeground(dis, gc, foreground.pixel);
  XDrawString(dis, double_buffer, gc, 0, xwa.height - 1, command, command_idx);

  if (sec)
  {
    XSetForeground(dis, gc, foreground.pixel);
    XDrawString(dis, double_buffer, gc, 0, font_height, sec->name,
                strlen(sec->name));

    char data[256] = {0};
    sprintf(data, "| BID: %.*f | ASK: %.*f | TS: %lu", sec->display_precision,
            (double)sec->best_bid / pow10[sec->display_precision],
            sec->display_precision,
            (double)sec->best_ask / pow10[sec->display_precision],
            sec->last_update);

    XDrawString(dis, double_buffer, gc,
                (strlen(sec->name) + 1) * font_info->per_char->width,
                font_height, data, strlen(data));

    int yaxis_offset = font_info->per_char->width * 10;
    XDrawLine(dis, double_buffer, gc, xwa.width - yaxis_offset, font_height,
              xwa.width - yaxis_offset, xwa.height - font_height);

    uint32_t num_candles =
        (uint32_t)floor((xwa.width - yaxis_offset) / candle_width);

    struct chart *cht = sec->chart;
    uint32_t min_value = 1215752190;
    uint32_t max_value = 0;
    size_t start_idx = 0;

    /* determine number of candles based on drawing width */
    if (cht->cur_candle_idx >= num_candles)
    {
      start_idx = cht->cur_candle_idx - num_candles;
    }

    /* find min and max of viewable candles */
    for (uint32_t i = start_idx; i <= cht->cur_candle_idx; ++i)
    {
      if (cht->candles[i].volume != 0)
      {
        if (cht->candles[i].high > max_value)
        {
          max_value = cht->candles[i].high;
        }
        if (cht->candles[i].low < min_value)
        {
          min_value = cht->candles[i].low;
        }
      }
    }

    /* modify the min and max value to always be divisible by the spread */
    uint32_t spread = sec->best_ask - sec->best_bid;
    if (spread != 0)
    {
      max_value += (spread - (max_value % spread));
      min_value -= (max_value % spread);
    }

    /* create a linear equation to map price -> pixel y value */
    struct linear_equation *price_to_pixel = NULL;
    price_to_pixel = linear_equation_new(max_value, font_height, min_value,
                                         xwa.height - font_height);

    /* create an equation to map pixel y value to price */
    struct linear_equation *pixel_to_price = NULL;
    pixel_to_price = linear_equation_new(font_height, max_value,
                                         xwa.height - font_height, min_value);

    XSetForeground(dis, gc, foreground.pixel);
    XSetFillStyle(dis, gc, FillSolid);

    /* understand how many pips our font height takes up */
    int64_t pips = (linear_equation_eval(pixel_to_price, font_height)) -
                   linear_equation_eval(pixel_to_price, font_height * 2);

    /*
     * if the number of pips the hight of our font covers is < 1
     * force the drawing to draw every pip from min to max
     */
    if (pips <= 1)
    {
      pips = 1;
    }

    /* loop through the min and max and draw the tick marks */
    for (uint32_t i = min_value; i <= max_value; i += pips)
    {
      char level[20] = {0};

      /* format the price to a specified value */
      sprintf(level, "-%.*f", sec->display_precision,
              (double)i / pow10[sec->display_precision]);

      /* draw the tick markets on the side */
      XDrawString(dis, double_buffer, gc,
                  xwa.width - yaxis_offset + font_info->per_char->width,
                  linear_equation_eval(price_to_pixel, i), level,
                  strlen(level));
    }

    /* draw a box to highlight bid price */
    XSetForeground(dis, gc, foreground.pixel);
    XSetFillStyle(dis, gc, FillSolid);
    XFillRectangle(dis, double_buffer, gc, xwa.width - yaxis_offset,
                   linear_equation_eval(price_to_pixel, sec->best_bid) -
                       (font_info->ascent) + (font_info->descent + 1),
                   yaxis_offset, font_height);

    if (spread != 0)
    {
      /* draw a box to highlight the ask price */
      XSetForeground(dis, gc, foreground.pixel);
      XSetFillStyle(dis, gc, FillSolid);
      XFillRectangle(dis, double_buffer, gc, xwa.width - yaxis_offset,
                     linear_equation_eval(price_to_pixel, sec->best_ask) -
                         (font_info->ascent) + (font_info->descent + 1),
                     yaxis_offset, font_height);
    }

    /* place holder string for the tick marks */
    char level[20] = {0};

    /* draw the bid price */
    XSetForeground(dis, gc, background.pixel);
    XSetFillStyle(dis, gc, FillSolid);
    sprintf(level, "- %.*f", sec->display_precision,
            (double)sec->best_bid / pow10[sec->display_precision]);
    XDrawString(dis, double_buffer, gc, xwa.width - yaxis_offset,
                linear_equation_eval(price_to_pixel, sec->best_bid) +
                    font_info->descent + 1,
                level, strlen(level));

    if (spread != 0)
    {
      /* draw the ask price */
      XSetForeground(dis, gc, background.pixel);
      XSetFillStyle(dis, gc, FillSolid);

      sprintf(level, "- %.*f", sec->display_precision,
              (double)sec->best_ask / pow10[sec->display_precision]);

      XDrawString(dis, double_buffer, gc, xwa.width - yaxis_offset,
                  linear_equation_eval(price_to_pixel, sec->best_ask) +
                      font_info->descent + 1,
                  level, strlen(level));
    }

    /* draw each visible candle */
    for (uint32_t i = start_idx + 1; i <= cht->cur_candle_idx; ++i)
    {

      if (cht->candles[i].high != cht->candles[i].low)
      {
        /* draw the whick of the candle */
        XSetForeground(dis, gc, foreground.pixel);
        XSetFillStyle(dis, gc, FillSolid);

        XDrawLine(dis, double_buffer, gc,
                  ((i - start_idx - 1) * (candle_width) +
                   (font_info->per_char->width / 2)),
                  linear_equation_eval(price_to_pixel, cht->candles[i].high),
                  ((i - start_idx - 1) * (candle_width) +
                   (font_info->per_char->width / 2)),
                  linear_equation_eval(price_to_pixel, cht->candles[i].low) -
                      1);
      }

      if (cht->candles[i].volume != 0)
      {
        /* draw the body of the candle */
        if (cht->candles[i].open > cht->candles[i].close)
        {
          XSetForeground(dis, gc, downward.pixel);
          XSetFillStyle(dis, gc, FillSolid);

          int64_t candle_height =
              linear_equation_eval(price_to_pixel, cht->candles[i].close) -
              linear_equation_eval(price_to_pixel, cht->candles[i].open);
          XFillRectangle(
              dis, double_buffer, gc, (i - start_idx - 1) * (candle_width),
              linear_equation_eval(price_to_pixel, cht->candles[i].open),
              (font_info->per_char->width) + 1, candle_height);
        }
        else if (cht->candles[i].open < cht->candles[i].close)
        {
          XSetForeground(dis, gc, upward.pixel);
          XSetFillStyle(dis, gc, FillSolid);

          int64_t candle_height =
              linear_equation_eval(price_to_pixel, cht->candles[i].open) -
              linear_equation_eval(price_to_pixel, cht->candles[i].close);
          XFillRectangle(
              dis, double_buffer, gc, (i - start_idx - 1) * (candle_width),
              linear_equation_eval(price_to_pixel, cht->candles[i].close),
              (font_info->per_char->width) + 1, candle_height);
        }
        else if (cht->candles[i].open == cht->candles[i].close)
        {
          XSetForeground(dis, gc, foreground.pixel);
          XSetFillStyle(dis, gc, FillSolid);
          XDrawLine(
              dis, double_buffer, gc, (i - start_idx - 1) * (candle_width),
              linear_equation_eval(price_to_pixel, cht->candles[i].close),
              ((i - start_idx - 1) * (candle_width)) +
                  (font_info->per_char->width),
              linear_equation_eval(price_to_pixel, cht->candles[i].close));
        }

        struct chart_object *analysis_iter = cht->candles[i].analysis_list;

        int draw_height = 1;

        while (analysis_iter)
        {
          switch (analysis_iter->object_type)
          {
          case CHART_OBJECT_TEXT: {
            struct chart_object_t_text *analysis_object = analysis_iter->value;
            XSetForeground(dis, gc, foreground.pixel);
            XSetFillStyle(dis, gc, FillSolid);
            XDrawString(
                dis, double_buffer, gc,
                (i - start_idx - 1) * (candle_width) + 1,
                linear_equation_eval(price_to_pixel, cht->candles[i].low) +
                    (font_height * draw_height),
                &(analysis_object->TEXT), 1);
            draw_height += 1;
            break;
          }
          case CHART_OBJECT_LINE: {
            struct chart_object_t_line *analysis_object = analysis_iter->value;
            XSetForeground(dis, gc, orange.pixel);
            XSetFillStyle(dis, gc, FillSolid);
            XDrawLine(
                dis, double_buffer, gc,
                ((analysis_object->start - start_idx - 1) * (candle_width)) +
                    (font_info->per_char->width / 2),
                linear_equation_eval(price_to_pixel,
                                     analysis_object->start_price),
                (analysis_object->end - start_idx - 1) * (candle_width) +
                    (font_info->per_char->width / 2),
                linear_equation_eval(price_to_pixel,
                                     analysis_object->end_price));
            break;
          }
          }
          analysis_iter = analysis_iter->next;
        }
      }
    }

    linear_equation_free(&price_to_pixel);
  }

  XCopyArea(dis, double_buffer, win, gc, 0, 0, xwa.width, xwa.height, 0, 0);
  XFlush(dis);
  pthread_mutex_unlock(&draw_mutex);
}

int
client_start()
{

  /* tell X11 that there are multiple threads to this program */
  XInitThreads();

  /* setup the drawing window, color scheme etc... */
  _client_init();

  /* declare an XEvent */
  XEvent event;

  /*
   * Loop through each event at a time
   */
  while (globals_continue(NULL))
  {

    while (XPending(dis))
    {
      /*
       * Block until next event
       */
      XNextEvent(dis, &event);

      switch (event.type)
      {
      case Expose:
        _redraw();
        break;
      case KeyPress:
        if (_process_keypress(event))
        {
          finished_displaying = true;
        }
        _redraw();
        break;
      case ButtonPress:
        break;
      case ConfigureNotify:
        break;
      default:
        break;
      }
    }
  }

  finished_displaying = true;

  pthread_mutex_lock(&draw_mutex);
  XFreeGC(dis, gc);
  XDestroyWindow(dis, win);
  XCloseDisplay(dis);
  pthread_mutex_unlock(&draw_mutex);

  return 0;
}

int
client_getdisplay(Display **display)
{
  if (!finished_displaying && globals_continue(NULL))
  {
    if (dis)
    {
      *display = dis;
      return true;
    }
    else
    {
      *display = NULL;
      return false;
    }
  }
  else
  {
    return false;
  }
}

int
client_getwindow(Window *window)
{
  if (!finished_displaying && globals_continue(NULL))
  {
    *window = win;
    return true;
  }
  else
  {
    return false;
  }
}

void
client_redraw()
{
  if (!finished_displaying)
  {
    _redraw();
  }
}
