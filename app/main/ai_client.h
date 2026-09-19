#pragma once

#include <stdbool.h>

/* Called from a FreeRTOS task context (NOT the LVGL task).
 * The callback must take the LVGL mutex itself before touching widgets. */
typedef void (*ai_reply_cb)(const char *reply, bool ok, void *user_data);

void ai_client_init(void);
void ai_client_ask(const char *question, ai_reply_cb cb, void *user_data);
bool ai_client_configured(void);
bool ai_client_busy(void);
