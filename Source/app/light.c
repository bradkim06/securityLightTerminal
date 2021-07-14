#include "app.h"

light_t light;

void initLightConfig(void)
{
	memset(&light, 0, sizeof(light_t));
	light.mode = LMODE_INTERNAL;
	light.timeBase = SUNRISE_BASE;
}
