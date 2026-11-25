// SPDX-License-Identifier: MIT
// rclsuru の C の包み。
// する の外部の関数は構造体を値で受け渡せないので、値で受け渡す rcl の関数
// (rcl_get_zero_initialized_* や *_get_default_options など)だけをここで包み、
// 番地(ポインタ)にして渡します。番地を受け取る rcl の関数は する から直接呼びます。
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <rcl/rcl.h>
#include <rcl/event_callback.h>
#include <rcl/logging.h>
#include <rcl/logging_rosout.h>
#include <rcl/client.h>
#include <rcl/graph.h>
#include <rcl/service.h>
#include <rcl/arguments.h>
#include <rcl/time.h>
#include <rcutils/logging.h>
#include <rcl_yaml_param_parser/parser.h>
#include <rcl_yaml_param_parser/types.h>
#include <rcutils/error_handling.h>
#include <rmw/serialized_message.h>
#include <rmw/qos_profiles.h>

// --- エラー ---

static char rclsuru_error_buffer[RCUTILS_ERROR_MESSAGE_MAX_LENGTH];

// 直前の rcl のエラーの文(なければ「」)。読んだら消す。
const char *rclsuru_last_error(void) {
  if (!rcutils_error_is_set()) {
    return "";
  }
  rcutils_error_string_t s = rcutils_get_error_string();
  strncpy(rclsuru_error_buffer, s.str, sizeof(rclsuru_error_buffer) - 1);
  rclsuru_error_buffer[sizeof(rclsuru_error_buffer) - 1] = '\0';
  rcutils_reset_error();
  return rclsuru_error_buffer;
}

// --- context と初期化 ---

rcl_context_t *rclsuru_context_new(void) {
  rcl_context_t *p = malloc(sizeof(rcl_context_t));
  if (p) *p = rcl_get_zero_initialized_context();
  return p;
}

void rclsuru_context_delete(rcl_context_t *p) { free(p); }

// rcl_init_options_init まで済ませた初期化の設定。失敗なら NULL。
rcl_init_options_t *rclsuru_init_options_new(void) {
  rcl_init_options_t *p = malloc(sizeof(rcl_init_options_t));
  if (!p) return NULL;
  *p = rcl_get_zero_initialized_init_options();
  if (rcl_init_options_init(p, rcl_get_default_allocator()) != RCL_RET_OK) {
    free(p);
    return NULL;
  }
  return p;
}

void rclsuru_init_options_delete(rcl_init_options_t *p) {
  if (!p) return;
  (void)rcl_init_options_fini(p);
  free(p);
}

// ログ(rcutils のログと /rosout)の初期化。rcl_init の後に1回。
int64_t rclsuru_logging_init(rcl_context_t *context) {
  rcl_allocator_t a = rcl_get_default_allocator();
  return rcl_logging_configure(&context->global_arguments, &a);
}

// --- ログ ---

// ノードのロガーの名前でログを出す(コンソール・/rosout・ログファイル)。
// 重さは 10 DEBUG・20 INFO・30 WARN・40 ERROR・50 FATAL。rcutils のマクロは使えないので関数で包む
void rclsuru_log(const rcl_node_t *node, int64_t severity, const char *msg)
{
  const char *name = rcl_node_get_logger_name(node);
  if (!name) {
    name = "";
  }
  if (rcutils_logging_logger_is_enabled_for(name, (int)severity)) {
    // /rosout の出力は場所を必ず読むので、空の場所を渡す(する の関数と行は分からない)
    rcutils_log_location_t where = {"", "", 0};
    rcutils_log(&where, (int)severity, name, "%s", msg);
  }
}

// /rosout の発行者(rclcpp と同じく、rcl_node_init の後に作り、rcl_node_fini の前に片付ける)
int64_t rclsuru_rosout_init(rcl_node_t *node)
{
  if (!rcl_logging_rosout_enabled() || !rcl_node_get_options(node)->enable_rosout) {
    return RCL_RET_OK;
  }
  return rcl_logging_rosout_init_publisher_for_node(node);
}

int64_t rclsuru_rosout_fini(rcl_node_t *node)
{
  if (!rcl_logging_rosout_enabled() || !rcl_node_get_options(node)->enable_rosout) {
    return RCL_RET_OK;
  }
  return rcl_logging_rosout_fini_publisher_for_node(node);
}

// --- 時計 ---

// 種類は 1 ROS_TIME・2 SYSTEM_TIME・3 STEADY_TIME。失敗なら NULL
rcl_clock_t *rclsuru_clock_new(int64_t type)
{
  rcl_clock_t *p = malloc(sizeof(rcl_clock_t));
  rcl_allocator_t a = rcl_get_default_allocator();
  if (p && rcl_clock_init((rcl_clock_type_t)type, p, &a) != RCL_RET_OK) {
    rcutils_reset_error();
    free(p);
    return NULL;
  }
  return p;
}

// いまの時刻(ナノ秒)。失敗なら -1
int64_t rclsuru_clock_now(rcl_clock_t *p)
{
  rcl_time_point_value_t t = 0;
  if (rcl_clock_get_now(p, &t) != RCL_RET_OK) {
    rcutils_reset_error();
    return -1;
  }
  return t;
}

// /clock で進める(sim time)。有効にしてから rclsuru_clock_set で時刻を入れる
int64_t rclsuru_clock_enable_override(rcl_clock_t *p) { return rcl_enable_ros_time_override(p); }

int64_t rclsuru_clock_set(rcl_clock_t *p, int64_t ns) { return rcl_set_ros_time_override(p, ns); }

// --- 起動時のパラメータ(--ros-args -p・--params-file) ---

// コマンドラインのパラメータ(全部のノードの分)。なければ NULL。
rcl_params_t *rclsuru_param_overrides(rcl_context_t *c) {
  rcl_params_t *p = NULL;
  if (rcl_arguments_get_param_overrides(&c->global_arguments, &p) != RCL_RET_OK) {
    rcutils_reset_error();
    return NULL;
  }
  return p;
}

void rclsuru_param_overrides_delete(rcl_params_t *p) {
  if (p) rcl_yaml_node_struct_fini(p);
}

// 「/」で区切った1つ分を比べる。* は1つ、** はいくつでも(0 個も)。
static int rclsuru_name_match(const char *pat, const char *name) {
  if (*pat == '/') pat++;
  if (*name == '/') name++;
  if (*pat == '\0') return *name == '\0';
  const char *pe = strchr(pat, '/');
  size_t pl = pe ? (size_t)(pe - pat) : strlen(pat);
  const char *rest_pat = pe ? pe : pat + pl;
  if (pl == 2 && strncmp(pat, "**", 2) == 0) {
    for (const char *n = name;; ) {
      if (rclsuru_name_match(rest_pat, n)) return 1;
      const char *ne = strchr(n, '/');
      if (!ne) return rclsuru_name_match(rest_pat, "");
      n = ne + 1;
      if (*n == '\0') return rclsuru_name_match(rest_pat, "");
    }
  }
  if (*name == '\0') return 0;
  const char *ne = strchr(name, '/');
  size_t nl = ne ? (size_t)(ne - name) : strlen(name);
  if (!(pl == 1 && *pat == '*') && (pl != nl || strncmp(pat, name, pl) != 0)) return 0;
  return rclsuru_name_match(rest_pat, ne ? ne : name + nl);
}

// ノード(完全な名前 fqn)の、名前 name のパラメータの値。後に書いたものが勝つ。なければ NULL。
const rcl_variant_t *rclsuru_param_find(const rcl_params_t *p, const char *fqn, const char *name) {
  if (!p) return NULL;
  const rcl_variant_t *found = NULL;
  for (size_t i = 0; i < p->num_nodes; i++) {
    if (!rclsuru_name_match(p->node_names[i], fqn)) continue;
    const rcl_node_params_t *np = &p->params[i];
    for (size_t j = 0; j < np->num_params; j++) {
      if (strcmp(np->parameter_names[j], name) == 0) found = &np->parameter_values[j];
    }
  }
  return found;
}

// ノードに当てはまるパラメータの名前を、改行でつないだもの(呼んだ側が free する)。
char *rclsuru_param_names(const rcl_params_t *p, const char *fqn) {
  size_t n = 1;
  if (p) {
    for (size_t i = 0; i < p->num_nodes; i++) {
      if (!rclsuru_name_match(p->node_names[i], fqn)) continue;
      for (size_t j = 0; j < p->params[i].num_params; j++) n += strlen(p->params[i].parameter_names[j]) + 1;
    }
  }
  char *out = calloc(1, n);
  if (!out || !p) return out;
  for (size_t i = 0; i < p->num_nodes; i++) {
    if (!rclsuru_name_match(p->node_names[i], fqn)) continue;
    for (size_t j = 0; j < p->params[i].num_params; j++) {
      strcat(out, p->params[i].parameter_names[j]);
      strcat(out, "\n");
    }
  }
  return out;
}

// 値の種類: 0 なし、1 bool、2 integer、3 double、4 string、5 byte の配列、6 bool の配列、
// 7 integer の配列、8 double の配列、9 string の配列(rcl_interfaces/ParameterType と同じ番号)
int64_t rclsuru_variant_type(const rcl_variant_t *v) {
  if (!v) return 0;
  if (v->bool_value) return 1;
  if (v->integer_value) return 2;
  if (v->double_value) return 3;
  if (v->string_value) return 4;
  if (v->byte_array_value) return 5;
  if (v->bool_array_value) return 6;
  if (v->integer_array_value) return 7;
  if (v->double_array_value) return 8;
  if (v->string_array_value) return 9;
  return 0;
}

int64_t rclsuru_variant_bool(const rcl_variant_t *v) { return *v->bool_value ? 1 : 0; }
int64_t rclsuru_variant_integer(const rcl_variant_t *v) { return *v->integer_value; }
double rclsuru_variant_double(const rcl_variant_t *v) { return *v->double_value; }
const char *rclsuru_variant_string(const rcl_variant_t *v) { return v->string_value; }

// 配列の長さ
int64_t rclsuru_variant_size(const rcl_variant_t *v) {
  switch (rclsuru_variant_type(v)) {
    case 5: return (int64_t)v->byte_array_value->size;
    case 6: return (int64_t)v->bool_array_value->size;
    case 7: return (int64_t)v->integer_array_value->size;
    case 8: return (int64_t)v->double_array_value->size;
    case 9: return (int64_t)v->string_array_value->size;
    default: return 0;
  }
}

// 配列の i 番目(0 から)
int64_t rclsuru_variant_byte_at(const rcl_variant_t *v, int64_t i) { return v->byte_array_value->values[i]; }
int64_t rclsuru_variant_bool_at(const rcl_variant_t *v, int64_t i) { return v->bool_array_value->values[i] ? 1 : 0; }
int64_t rclsuru_variant_integer_at(const rcl_variant_t *v, int64_t i) { return v->integer_array_value->values[i]; }
double rclsuru_variant_double_at(const rcl_variant_t *v, int64_t i) { return v->double_array_value->values[i]; }
const char *rclsuru_variant_string_at(const rcl_variant_t *v, int64_t i) { return v->string_array_value->data[i]; }

// --- ノード ---

rcl_node_t *rclsuru_node_new(void) {
  rcl_node_t *p = malloc(sizeof(rcl_node_t));
  if (p) *p = rcl_get_zero_initialized_node();
  return p;
}

void rclsuru_node_delete(rcl_node_t *p) { free(p); }

rcl_node_options_t *rclsuru_node_options_new(void) {
  rcl_node_options_t *p = malloc(sizeof(rcl_node_options_t));
  if (p) *p = rcl_node_get_default_options();
  return p;
}

void rclsuru_node_options_delete(rcl_node_options_t *p) {
  if (!p) return;
  (void)rcl_node_options_fini(p);
  free(p);
}

// --- QoS ---

// 名前の決まった QoS の写し。名前は default・sensor_data・services_default・parameters・
// parameter_events・system_default。知らない名前なら NULL。
rmw_qos_profile_t *rclsuru_qos_new(const char *name) {
  const rmw_qos_profile_t *src = NULL;
  if (strcmp(name, "default") == 0) src = &rmw_qos_profile_default;
  else if (strcmp(name, "sensor_data") == 0) src = &rmw_qos_profile_sensor_data;
  else if (strcmp(name, "services_default") == 0) src = &rmw_qos_profile_services_default;
  else if (strcmp(name, "parameters") == 0) src = &rmw_qos_profile_parameters;
  else if (strcmp(name, "parameter_events") == 0) src = &rmw_qos_profile_parameter_events;
  else if (strcmp(name, "system_default") == 0) src = &rmw_qos_profile_system_default;
  if (!src) return NULL;
  rmw_qos_profile_t *p = malloc(sizeof(rmw_qos_profile_t));
  if (p) *p = *src;
  return p;
}

void rclsuru_qos_delete(rmw_qos_profile_t *p) { free(p); }

// 決まりを変える。負の数なら変えない。
//   履歴: 0 は SYSTEM_DEFAULT、1 は KEEP_LAST、2 は KEEP_ALL。深さ は KEEP_LAST のときに残す数
//   信頼性: 0 は SYSTEM_DEFAULT、1 は RELIABLE、2 は BEST_EFFORT
//   永続性: 0 は SYSTEM_DEFAULT、1 は TRANSIENT_LOCAL、2 は VOLATILE
void rclsuru_qos_apply(rmw_qos_profile_t *p, int64_t history, int64_t depth, int64_t reliability,
                       int64_t durability) {
  if (history >= 0) p->history = (enum rmw_qos_history_policy_e)history;
  if (depth >= 0) p->depth = (size_t)depth;
  if (reliability >= 0) p->reliability = (enum rmw_qos_reliability_policy_e)reliability;
  if (durability >= 0) p->durability = (enum rmw_qos_durability_policy_e)durability;
}

// --- 発行者 ---

rcl_publisher_t *rclsuru_publisher_new(void) {
  rcl_publisher_t *p = malloc(sizeof(rcl_publisher_t));
  if (p) *p = rcl_get_zero_initialized_publisher();
  return p;
}

void rclsuru_publisher_delete(rcl_publisher_t *p) { free(p); }

// qos が NULL なら既定(深さ 10)。
rcl_publisher_options_t *rclsuru_publisher_options_new(const rmw_qos_profile_t *qos) {
  rcl_publisher_options_t *p = malloc(sizeof(rcl_publisher_options_t));
  if (!p) return NULL;
  *p = rcl_publisher_get_default_options();
  if (qos) p->qos = *qos;
  return p;
}

void rclsuru_publisher_options_delete(rcl_publisher_options_t *p) { free(p); }

// --- 購読者 ---

rcl_subscription_t *rclsuru_subscription_new(void) {
  rcl_subscription_t *p = malloc(sizeof(rcl_subscription_t));
  if (p) *p = rcl_get_zero_initialized_subscription();
  return p;
}

void rclsuru_subscription_delete(rcl_subscription_t *p) { free(p); }

rcl_subscription_options_t *rclsuru_subscription_options_new(const rmw_qos_profile_t *qos) {
  rcl_subscription_options_t *p = malloc(sizeof(rcl_subscription_options_t));
  if (!p) return NULL;
  *p = rcl_subscription_get_default_options();
  if (qos) p->qos = *qos;
  return p;
}

void rclsuru_subscription_options_delete(rcl_subscription_options_t *p) {
  if (!p) return;
  (void)rcl_subscription_options_fini(p);
  free(p);
}

// --- 直列化したメッセージ ---

rmw_serialized_message_t *rclsuru_serialized_new(int64_t capacity) {
  rmw_serialized_message_t *p = malloc(sizeof(rmw_serialized_message_t));
  if (!p) return NULL;
  *p = rmw_get_zero_initialized_serialized_message();
  rcutils_allocator_t a = rcutils_get_default_allocator();
  if (rmw_serialized_message_init(p, (size_t)capacity, &a) != RMW_RET_OK) {
    free(p);
    return NULL;
  }
  return p;
}

void rclsuru_serialized_delete(rmw_serialized_message_t *p) {
  if (!p) return;
  (void)rmw_serialized_message_fini(p);
  free(p);
}

// 中身を data の n バイトにする(足りなければ広げる)。成功なら 0。
int64_t rclsuru_serialized_set(rmw_serialized_message_t *p, const uint8_t *data, int64_t n) {
  if ((size_t)n > p->buffer_capacity) {
    if (rmw_serialized_message_resize(p, (size_t)n) != RMW_RET_OK) return -1;
  }
  memcpy(p->buffer, data, (size_t)n);
  p->buffer_length = (size_t)n;
  return 0;
}

// 中身を n バイトにして(足りなければ広げ、0 で埋める)、先頭の番地を返す。失敗なら NULL。
uint8_t *rclsuru_serialized_prepare(rmw_serialized_message_t *p, int64_t n) {
  if ((size_t)n > p->buffer_capacity) {
    if (rmw_serialized_message_resize(p, (size_t)n) != RMW_RET_OK) return NULL;
  }
  memset(p->buffer, 0, (size_t)n);
  p->buffer_length = (size_t)n;
  return p->buffer;
}

uint8_t *rclsuru_serialized_buffer(const rmw_serialized_message_t *p) { return p->buffer; }

int64_t rclsuru_serialized_length(const rmw_serialized_message_t *p) {
  return (int64_t)p->buffer_length;
}

// --- サービス ---

rcl_service_t *rclsuru_service_new(void) {
  rcl_service_t *p = malloc(sizeof(rcl_service_t));
  if (p) *p = rcl_get_zero_initialized_service();
  return p;
}

void rclsuru_service_delete(rcl_service_t *p) { free(p); }

// qos が NULL なら既定(services_default)。
rcl_service_options_t *rclsuru_service_options_new(const rmw_qos_profile_t *qos) {
  rcl_service_options_t *p = malloc(sizeof(rcl_service_options_t));
  if (!p) return NULL;
  *p = rcl_service_get_default_options();
  if (qos) p->qos = *qos;
  return p;
}

void rclsuru_service_options_delete(rcl_service_options_t *p) { free(p); }

rcl_client_t *rclsuru_client_new(void) {
  rcl_client_t *p = malloc(sizeof(rcl_client_t));
  if (p) *p = rcl_get_zero_initialized_client();
  return p;
}

void rclsuru_client_delete(rcl_client_t *p) { free(p); }

rcl_client_options_t *rclsuru_client_options_new(const rmw_qos_profile_t *qos) {
  rcl_client_options_t *p = malloc(sizeof(rcl_client_options_t));
  if (!p) return NULL;
  *p = rcl_client_get_default_options();
  if (qos) p->qos = *qos;
  return p;
}

void rclsuru_client_options_delete(rcl_client_options_t *p) { free(p); }

// 要求の見出し(rmw_request_id_t)。続き番号は sequence_number。
rmw_request_id_t *rclsuru_request_id_new(void) { return calloc(1, sizeof(rmw_request_id_t)); }

void rclsuru_request_id_delete(rmw_request_id_t *p) { free(p); }

// 後で答えるために見出しを写す
rmw_request_id_t *rclsuru_request_id_copy(const rmw_request_id_t *p)
{
  rmw_request_id_t *q = malloc(sizeof(rmw_request_id_t));
  if (q) {
    *q = *p;
  }
  return q;
}

int64_t rclsuru_request_id_sequence(const rmw_request_id_t *p) { return p->sequence_number; }

// サーバーが出ていれば 1、いなければ 0、失敗なら負の数(rcl の戻り値の符号を変えたもの)。
int64_t rclsuru_server_is_available(const rcl_node_t *node, const rcl_client_t *client) {
  bool ok = false;
  rcl_ret_t r = rcl_service_server_is_available(node, client, &ok);
  if (r != RCL_RET_OK) return -(int64_t)r;
  return ok ? 1 : 0;
}

// --- 受け取りの付け足しの情報 ---

rmw_message_info_t *rclsuru_message_info_new(void) {
  rmw_message_info_t *p = malloc(sizeof(rmw_message_info_t));
  if (p) *p = rmw_get_zero_initialized_message_info();
  return p;
}

void rclsuru_message_info_delete(rmw_message_info_t *p) { free(p); }
