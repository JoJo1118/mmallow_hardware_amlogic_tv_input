//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTvGroup.h
//  @ Date : 2013-11
//  @ Author :
//
//


#if !defined(_CTVGROUP_H)
#define _CTVGROUP_H

#include <utils/Vector.h>
using namespace android;
// Group��ӦDTV�е�һ����Ŀ����
class CTvGroup {
public:
    CTvGroup();
    ~CTvGroup();
    // ȡ�����н�Ŀ������Ϣ
    static Vector<CTvGroup> selectByGroup();
    static void addGroup();
    static void deleteGroup();
    static void editGroup();
};

#endif  //_CTVGROUP_H
