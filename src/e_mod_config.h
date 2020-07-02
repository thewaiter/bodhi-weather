#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_CONFIG_H
#define E_MOD_CONFIG_H

#include <e.h>

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

#endif
#endif
