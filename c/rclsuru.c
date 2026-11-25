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

// 履歴: 1 は KEEP_LAST、2 は KEEP_ALL。深さは KEEP_LAST の数。
void rclsuru_qos_set_history(rmw_qos_profile_t *p, int64_t history, int64_t depth) {
  p->history = (enum rmw_qos_history_policy_e)history;
  p->depth = (size_t)depth;
}

// 信頼性: 1 は RELIABLE、2 は BEST_EFFORT。
void rclsuru_qos_set_reliability(rmw_qos_profile_t *p, int64_t v) {
  p->reliability = (enum rmw_qos_reliability_policy_e)v;
}

// 永続性: 1 は TRANSIENT_LOCAL、2 は VOLATILE。
void rclsuru_qos_set_durability(rmw_qos_profile_t *p, int64_t v) {
  p->durability = (enum rmw_qos_durability_policy_e)v;
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

uint8_t *rclsuru_serialized_buffer(const rmw_serialized_message_t *p) { return p->buffer; }

int64_t rclsuru_serialized_length(const rmw_serialized_message_t *p) {
  return (int64_t)p->buffer_length;
}

// --- 受け取りの付け足しの情報 ---

rmw_message_info_t *rclsuru_message_info_new(void) {
  rmw_message_info_t *p = malloc(sizeof(rmw_message_info_t));
  if (p) *p = rmw_get_zero_initialized_message_info();
  return p;
}

void rclsuru_message_info_delete(rmw_message_info_t *p) { free(p); }
