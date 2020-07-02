#ifndef FC_COMMON_H
#define FC_COMMON_H

#include <e.h>
#include "e_mod_config.h"

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

typedef struct _Instance  Instance;
typedef struct _Forecasts Forecasts;
typedef struct _Config Config;
typedef struct _Config_Item Config_Item; 

struct _Instance
{
   E_Gadcon_Client     *gcc;
   Evas_Object         *forecasts_obj;
   Forecasts           *forecasts;
   Ecore_Timer         *check_timer, *delay;
   Eina_Binbuf         *buffer;
   Eina_List           *handlers;
 
 struct
   {
      int  code;
      int  temp, temp_c, temp_f;
      int  feel, feel_c, feel_f;
      char update[52];
      char desc[256];
   } condition;
 
   struct
   {
      char temp, distance[3], pressure[3], speed[5];
   } units;
 
   struct
   {
      struct
      {
         int chill, chill_c, chill_f;
         int direction;
         int speed, speed_km, speed_mi;
      } wind;
 
      struct
      {
         int   humidity, rising;
         float pressure, pressure_km, pressure_mi; 
         float visibility, visibility_km, visibility_mi;
         float precip;
      } atmosphere;
 
      struct
      {
         char sunrise[9], sunset[9];
      } astronomy;
   } details;
 
   struct
   {
      char day[4];
      char date[12];
      int  low, high, low_c, high_c, low_f, high_f, code;
      char desc[256];
   } forecast[5];
 
   const char     *location;
   const char     *country;
   const char     *language;
   const char     *label;
   const char     *area;
 
   E_Gadcon_Popup *popup;
   Config_Item    *ci;
};

struct _Forecasts

{
   Instance    *inst;
   Evas_Object *forecasts_obj;
   Evas_Object *icon_obj;
};


 
#endif
