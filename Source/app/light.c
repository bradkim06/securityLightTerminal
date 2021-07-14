#include "app.h"

void initLightConfig(void)
{
	memset(&conf.light, 0, sizeof(light_t));
	conf.light.mode = LMODE_INTERNAL;
	conf.light.timeBase = CIVIL;
}
