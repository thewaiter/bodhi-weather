#include <e.h>
#include "utility.h"

char *
url_normalize_str(const char *str)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(str, NULL);

  Eina_Strbuf *buf;
  char *ret;

  buf = eina_strbuf_new();
  eina_strbuf_append(buf, str);
  eina_strbuf_replace_all(buf, " ", "%20");
  ret = eina_strbuf_string_steal(buf);
  eina_strbuf_string_free(buf);

  return ret;
} 

char *
lang_normalize_str(const char *str)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(str, NULL);

  Eina_Strbuf *buf;
  char *lang;

  buf = eina_strbuf_new();
  eina_strbuf_append(buf, str);
  eina_strbuf_remove(buf, 2, strlen(str));
  lang = eina_strbuf_string_steal(buf);
  eina_strbuf_string_free(buf);

  return lang;
} 
