#include <e.h>

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define D(x) //do { printf(__FILE__ ":%d:\t", __LINE__); printf x;
	     //fflush(stdout); } while(0)

#define _(str) dgettext(PACKAGE, str)

/* events sent to ecomp */
#define ECOMORPH_EVENT_MAPPED       0
#define ECOMORPH_EVENT_STATE        1
#define ECOMORPH_EVENT_DESK         2
#define ECOMORPH_EVENT_RESTART      3
#define ECOMORPH_EVENT_GRAB         4
#define ECOMORPH_EVENT_MOVE         5
#define ECOMORPH_EVENT_MOVE_RESIZE  6
#define ECOMORPH_EVENT_FOCUS        7

/* events sent from ecomp */
#define ECOMORPH_ECOMP_WINDOW_MOVE      100
#define ECOMORPH_ECOMP_WINDOW_ACTIVATE  101
#define ECOMORPH_ECOMP_WINDOW_STACKING  102
#define ECOMORPH_ECOMP_VIEWPORT         200
#define ECOMORPH_ECOMP_PLUGIN_END       210


#define ECOMORPH_WINDOW_STATE_INVISIBLE 0
#define ECOMORPH_WINDOW_STATE_VISIBLE 1

#define LIST_DATA_BY_MEMBER_FIND(_list, _member, _data, _result)	\
  {									\
    Eina_List *l;							\
    _result = NULL;							\
    EINA_LIST_FOREACH(_list, l, _result)				\
      if (_result && _result->_member == _data)			       	\
	break;								\
    if (_result && (_result->_member != _data)) _result = NULL;		\
  }

#define LIST_PUSH(_list, _data)			\
  if (_data)					\
    _list = eina_list_append(_list, _data);	\
  

#define LIST_REMOVE_DATA(_lll, _data)		\
  if (_data)					\
    _lll = eina_list_remove(_lll, _data);	\

  
#define MOD(a,b) ((a) < 0 ? ((b) - ((-(a) - 1) % (b))) - 1 : (a) % (b))

#include "eco_config.h"
#include "eco_actions.h"
#include "eco_event.h"

typedef struct _Config Config;
struct _Config 
{
   int dropshadow;
   int compscale;
   int composite;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

extern Ecore_X_Atom ECOMORPH_ATOM_MANAGED;
extern Ecore_X_Atom ECOMORPH_ATOM_PLUGIN;
#endif

