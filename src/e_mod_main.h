#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#ifdef ENABLE_NLS
# include <libintl.h>
# define D_(string) dgettext(PACKAGE, string)
#else
# define bindtextdomain(domain,dir)
# define bind_textdomain_codeset(domain,codeset)
# define D_(string) (string)
#endif

/* EINA_LOG support macros and global */
extern int _e_forecast_log_dom;
#undef DBG
#undef INF
#undef WRN
#undef ERR
#undef CRI
#define DBG(...)            EINA_LOG_DOM_DBG(_e_forecast_log_dom, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(_e_forecast_log_dom, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(_e_forecast_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(_e_forecast_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(_e_forecast_log_dom, __VA_ARGS__)

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

typedef struct _Config Config;
typedef struct _Config_Item Config_Item;
typedef struct _Popup Popup;

struct _Config
{
   E_Module *module;
   int version;
   E_Config_Dialog *config_dialog;
   Eina_List *instances;
   Eina_List *items;
   E_Menu *menu;
};

struct _Config_Item
{
   const char *id;
   double poll_time;
   double days;
   int degrees;
   const char *host, *code, *lang, *label;
   int show_text;
   int popup_on_hover;
};


EAPI void *e_modapi_init(E_Module *m);
EAPI int   e_modapi_shutdown(E_Module *m __UNUSED__);
EAPI int   e_modapi_save(E_Module *m __UNUSED__);

void _config_forecasts_module(Config_Item *ci);
void _forecasts_config_updated(Config_Item *ci);

extern Config *forecasts_config;

#endif
