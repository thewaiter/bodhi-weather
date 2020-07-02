#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "common.h"
#include "e_mod_config.h"


#define DEGREES_F 0
#define DEGREES_C 1

/* Macros used for config file versioning */
/* You can increment the EPOCH value if the old configuration is not
 * compatible anymore, it creates an entire new one.
 * You need to increment GENERATION when you add new values to the
 * configuration file but is not needed to delete the existing conf  */
#define MOD_CONFIG_FILE_EPOCH 1
#define MOD_CONFIG_FILE_GENERATION 0
#define MOD_CONFIG_FILE_VERSION    ((MOD_CONFIG_FILE_EPOCH * 1000000) + MOD_CONFIG_FILE_GENERATION)

/* Setup the E Module Version, Needed to check if module can run. */
/* The version is stored at compilation time in the module, and is checked
 * by E in order to know if the module is compatible with the actual version */
EAPI extern E_Module_Api e_modapi;


EAPI void *e_modapi_init(E_Module *m);
EAPI int   e_modapi_shutdown(E_Module *m __UNUSED__);
EAPI int   e_modapi_save(E_Module *m __UNUSED__);

void _config_forecasts_module(Config_Item *ci);
void _fc_config_updated(Config_Item *ci);

extern Config *forecasts_config;

#endif
