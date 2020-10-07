#include "oanda.h"

static const char V3_ACCOUNTS_FMT[] = "/v3/accounts";

/*
static size_t
_oanda_timetots(char *str)
{
  double val = strtod(str, NULL);
  val *= 1000000000;
  return (size_t)val;
}

static char *
_oanda_first_id(char *response)
{
  __json_value _root = json_parse(response);
  __json_object _accounts = json_get_object(_root);
  __json_array _account = json_get_array(hashmap_get("accounts", _accounts));
  __json_object _id = json_get_object(_account->val);
  __json_string id = strdup(json_get_string(hashmap_get("id", _id)));

  json_free(_root);
  return id;
}

static char *
_oanda_gen_currency_list(char *response, int *num_instruments)
{
  __json_value _root = json_parse(response);
  __json_object _instruments = json_get_object(_root);
  __json_array instruments =
      json_get_array(hashmap_get("instruments", _instruments));

  char *currency_list = malloc(1);
  currency_list[0] = '\x0';
  size_t total_len = 0;

  while (instruments) {
    __json_object instrument = json_get_object(instruments->val);
    char *name = json_get_string(hashmap_get("name", instrument));
    int pip_location =
        (int)*(json_get_number(hashmap_get("pipLocation", instrument)));
    int display_precision =
        (int)*(json_get_number(hashmap_get("displayPrecision", instrument)));

    struct security *sec = security_new(name, pip_location, display_precision);
    exchange_put(name, sec);

    (*num_instruments) += 1;

    total_len += 8;
    currency_list = realloc(currency_list, total_len + 1);
    if (currency_list) {
      strcat(currency_list, name);
      strcat(currency_list, ",");
      currency_list[total_len] = '\x0';
    }

    instruments = instruments->nxt;
  }
  currency_list[total_len - 1] = '\x0';

  json_free(_root);
  return currency_list;
}
*/
void
exchanges_oanda_init(char *key)
{

  // create a reusable record for openssl read
  struct tls_session *master_session = NULL;
  if (tls_session_new(OANDA_API_ROOT, "443", &master_session) != STATUS_OK) {
    pprint_error("%s", "|");
    exit(1);
  }

  struct http11request *request = NULL;
  if (http11request_new(master_session, &request) != STATUS_OK) {
    pprint_error("%s", "|");
    exit(1);
  }

  request->stub = strdup(V3_ACCOUNTS_FMT);
  if (request->stub == NULL) {
    // TODO
  }

  char *authtoken = malloc(strlen("Bearer ") + strlen(key) + 1);
  memcpy(authtoken, "Bearer \x0", 8);
  strcat(authtoken, key);

  hashmap_put("Authorization", authtoken, request->headers);
  // hashmap_put("Accept-Datetime-Format", "UNIX", request->headers);
  hashmap_put("User-Agent", "crochet", request->headers);
  hashmap_put("Content-Type", "application/json", request->headers);
  hashmap_put("Accept", "*/*", request->headers);

  char *response = NULL;
  http11request_push(request, &response);

  /*
  char *response = NULL;
  http_get_request(master_session, "/v3/accounts", &response);

  char *id = _oanda_first_id(response);
  free(response);

  int get_instrument_size = 0;
  get_instrument_size =
      snprintf(NULL, 0, "/v3/accounts/%s/instruments", id) + 1;

  char *get_instrument_str = malloc((size_t)get_instrument_size * sizeof(char));
  sprintf(get_instrument_str, "/v3/accounts/%s/instruments", id);

  http_get_request(master_session, get_instrument_str, &response);
  free(get_instrument_str);

  int number_monitored = 0;
  char *instrument_update_end =
      _oanda_gen_currency_list(response, &number_monitored);

  pprint_info("oanda: loaded %d symbols", number_monitored);

  char *instrument_update_beg = "/v3/accounts/%s/pricing?instruments=";
  char *instrument_update_full = NULL;

  instrument_update_full = calloc(strlen(id) + strlen(instrument_update_beg) +
          strlen(instrument_update_end) + 2,
      sizeof(char));
  sprintf(instrument_update_full, "/v3/accounts/%s/pricing?instruments=%s", id,
      instrument_update_end);

  free(response);
  response = NULL;

  char *poll_request_cached =
      http_get_request_generate(master_session, instrument_update_full);
  int poll_request_cached_size = (int)strlen(poll_request_cached);

  int num_messages = 0;
  int num_valid_updates = 0;

  struct timespec start_time;
  struct timespec end_time;

#if defined(__FreeBSD__)
  clock_gettime(CLOCK_REALTIME_PRECISE, &start_time);
#else
  clock_gettime(CLOCK_REALTIME, &start_time);
#endif

  __json_value _response_root = NULL;

  response = NULL;
  size_t allocated = 0;
  while (globals_continue(NULL)) {
    http_get_request_cached(master_session, poll_request_cached,
        poll_request_cached_size, &response, &allocated);

    if (response == NULL) {
      pprint_warn("oanda: cloudflare disconnected reconnecting...");
      httpwss_session_free(master_session);
      master_session = httpwss_session_new(OANDA_API_ROOT, "443");
      master_session->hashauth = true;
      master_session->authkey = key;
      response = NULL;
      allocated = 0;
      continue;
    }

    if (_response_root == NULL) {
      _response_root = json_parse(response);
    } else {
      size_t idx = 0;
      json_parse_cached(response, &idx, _response_root);
    }
    __json_object _response_data = json_get_object(_response_root);
    __json_array _prices =
        json_get_array(hashmap_get("prices", _response_data));

    while (_prices) {
      __json_object _price_object = json_get_object(_prices->val);
      __json_string _price_instrument =
          json_get_string(hashmap_get("instrument", _price_object));
      __json_string price_update_time =
          json_get_string(hashmap_get("time", _price_object));

      size_t latest_timestamp = _oanda_timetots(price_update_time);

      __json_array _bids = json_get_array(hashmap_get("bids", _price_object));
      __json_array _asks = json_get_array(hashmap_get("asks", _price_object));

      __json_object _best_bid_bucket = json_get_object(_bids->val);
      __json_object _best_ask_bucket = json_get_object(_asks->val);

      __json_string best_bid =
          json_get_string(hashmap_get("price", _best_bid_bucket));
      __json_string best_ask =
          json_get_string(hashmap_get("price", _best_ask_bucket));

      struct security *working_sec = exchange_get(_price_instrument);

      if (security_update(working_sec, latest_timestamp, best_bid, best_ask)) {
        num_valid_updates += 1;
      }

      _prices = _prices->nxt;
    }

    num_messages += 1;

#if defined(__FreeBSD__)
    clock_gettime(CLOCK_REALTIME_PRECISE, &end_time);
#else
    clock_gettime(CLOCK_REALTIME, &end_time);
#endif

    if (end_time.tv_sec - start_time.tv_sec >=
        (int)OANDA_PRINT_NTERVAL_SECONDS) {

      pprint_info("oanda: requests=%lu updates=%lu",
          num_messages * number_monitored, num_valid_updates);

      num_messages = 0;
      num_valid_updates = 0;
#if defined(__FreeBSD__)
      clock_gettime(CLOCK_REALTIME_PRECISE, &start_time);
      clock_gettime(CLOCK_REALTIME_PRECISE, &end_time);
#else
      clock_gettime(CLOCK_REALTIME, &start_time);
      clock_gettime(CLOCK_REALTIME, &end_time);
#endif
    }
  }

  pprint_info("cleaning up exchange oanda...");
  free(response);
  json_free(_response_root);
  free(instrument_update_full);
  free(instrument_update_end);
  free(poll_request_cached);
  free(id);
  httpwss_session_free(master_session);
  */

  tls_session_free(&master_session);
}
