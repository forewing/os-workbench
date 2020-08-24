/*
 * File: dlist.h
 * Author: EasyAI
 * Email: easyai@outlook.com
 * Created Date: 2017-12-08 14:00:43
 * ------
 * Last Modified: 2019-03-20 19:09:33
 * Modified By: ***REMOVED***-***REMOVED*** ( ***REMOVED*** )
 */

#ifndef __DLIST_H__
#define __DLIST_H__

// Head node is only for identify, must not store data
#define __DLIST_INIT(__list__, __mnext__, __mprev__) \
    {                                                \
        (__list__)->__mnext__ = __list__;            \
        (__list__)->__mprev__ = __list__;            \
    }

#define __DLIST_EMPTY(__list__, __mnext__, __mprev__) \
    (((__list__)->__mprev__ == __list__ && (__list__)->__mnext__ == __list__) ? 1 : 0)

#define __DLIST_INSERT(__pos__, __mnext__, __mprev__, __node__) \
    {                                                           \
        (__pos__)->__mnext__->__mprev__ = __node__;             \
        (__node__)->__mnext__ = (__pos__)->__mnext__;           \
        (__node__)->__mprev__ = __pos__;                        \
        (__pos__)->__mnext__ = __node__;                        \
    }

#define __DLIST_DELETE(__list__, __pos__, __mnext__, __mprev__)     \
    {                                                               \
        if (!__DLIST_EMPTY(__list__, __mnext__, __mprev__)) {       \
            (__pos__)->__mnext__->__mprev__ = (__pos__)->__mprev__; \
            (__pos__)->__mprev__->__mnext__ = (__pos__)->__mnext__; \
            (__pos__)->__mnext__ = NULL;                            \
            (__pos__)->__mprev__ = NULL;                            \
        }                                                           \
    }

#define __DLIST_ADD_TAIL(__list__, __mnext__, __mprev__, __node__) \
    __DLIST_INSERT(__list__, __mprev__, __mnext__, __node__)

#define __DLIST_ADD_HEAD(__list__, __mnext__, __mprev__, __node__) \
    __DLIST_INSERT(__list__, __mnext__, __mprev__, __node__)

#define __DLIST_FOREACH(__list__, __mnext__, __ptr__) \
    for (__ptr__ = (__list__)->__mnext__; __ptr__ != __list__; __ptr__ = __ptr__->__mnext__)

#define __DLIST_FIND_NODE(__list__, __mnext__, __ret__, __key__, __cmpfunc__) \
    {                                                                         \
        __DLIST_FOREACH(__list__, __mnext__, __ret__)                         \
        if (__cmpfunc__(__key__, __ret__))                                    \
            break;                                                            \
        if (__ret__ == __list__)                                              \
            __ret__ = NULL;                                                   \
    }

#endif