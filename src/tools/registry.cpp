// registry.cpp — the ONLY central edit when adding a tool: append one line.
// Order matters: index = bit in gToolsMask. Never reorder, only append.
#include "tool_api.h"

extern const ToolDef Tool_get_status;
extern const ToolDef Tool_gpio_write;
extern const ToolDef Tool_gpio_read;
extern const ToolDef Tool_adc_read;
extern const ToolDef Tool_wifi_scan;
extern const ToolDef Tool_mem_save;
extern const ToolDef Tool_mem_read;
extern const ToolDef Tool_restart_device;
extern const ToolDef Tool_blink;
extern const ToolDef Tool_web_fetch;

const ToolDef* TOOL_REGISTRY[] = {
  &Tool_get_status,
  &Tool_gpio_write,
  &Tool_gpio_read,
  &Tool_adc_read,
  &Tool_wifi_scan,
  &Tool_mem_save,
  &Tool_mem_read,
  &Tool_restart_device,
  &Tool_blink,
  &Tool_web_fetch,
};

const int TOOL_COUNT = sizeof(TOOL_REGISTRY) / sizeof(TOOL_REGISTRY[0]);
