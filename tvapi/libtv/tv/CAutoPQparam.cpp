#include "CAutoPQparam.h"
#include "../tvsetting/CTvSetting.h"
#include "../tvconfig/tvconfig.h"
#include "../tvutils/tvutils.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <linux/fb.h>
#include <stdlib.h>
#include <cutils/properties.h>

#include <xxxconfig.h>

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "CAutoPQ"
#endif

CAutoPQparam::CAutoPQparam( CVpp *mVpp, CTvin *pTvin, CAv *mAv)
{
    myVpp = mVpp;
    myTvin = pTvin;
    myAv = mAv;
    preFmtType = 0;
    curFmtType = 0;
    autofreq_checkcount = 0;
    autofreq_checkflag = 0;
    mAutoPQ_OnOff_Flag = false;
}

CAutoPQparam::~CAutoPQparam()
{
    mAutoPQ_OnOff_Flag = false;
}

bool CAutoPQparam::isAutoPQing()
{
    return mAutoPQ_OnOff_Flag;
}

void CAutoPQparam::startAutoPQ( tv_source_input_type_t source_type )
{
#ifndef CC_PROJECT_DISABLE_AUTO_PQ
    mAutoPQSource = source_type;

    LOGD("---------startAutoPQParameters  --------mAutoPQ_OnOff_Flag  = %d", mAutoPQ_OnOff_Flag);
    if (!mAutoPQ_OnOff_Flag) {
        mAutoPQ_OnOff_Flag = true;
        this->run();
    }
#else
    LOGD("AutoPQparam disable.\n");
#endif
}

void CAutoPQparam::stopAutoPQ()
{
#ifndef CC_PROJECT_DISABLE_AUTO_PQ
    LOGD("---------stopAutoPQParameters  -------- mAutoPQ_OnOff_Flag = %d", mAutoPQ_OnOff_Flag);
    if (mAutoPQ_OnOff_Flag) {
        mAutoPQ_OnOff_Flag = false;
    }
#else
    LOGD("AutoPQparam disable.\n");
#endif
}

/**
TVIN_SIG_FMT_HDMI_720X480P_60HZ  = 0x402            nodeVal<900
TVIN_SIG_FMT_HDMI_1920X1080P_60HZ = 0x40a           900<nodeVal<2000
TVIN_SIG_FMT_HDMI_3840_2160_00HZ = 0x445            nodeVal>2000
*/
int CAutoPQparam::adjustPQparameters()
{
    int fd = -1;
    int nodeVal = 0, ret = 0;
    int new_frame_count = 0;
    float frame_rate = 0;
    float frame_rate_ave = 0;
    char s[21];
    char str[10];
    tvin_sig_fmt_e sig_fmt;
    is_3d_type_t _3d_type = INDEX_2D;
    tvin_trans_fmt trans_fmt = TVIN_TFMT_2D;

    fd = open("/sys/module/amvideo/parameters/new_frame_count", O_RDONLY);
    if (fd <= 0) {
        LOGE("open /sys/module/amvideo/parameters/new_frame_count  ERROR!!error = -%s- \n", strerror ( errno ));
        return -1;
    }
    memset(s, 0, sizeof(s));
    read(fd, s, sizeof(s));
    close(fd);
    new_frame_count = atoi(s);

    if (new_frame_count != 0) {

        fd = open("/sys/class/video/frame_height", O_RDONLY);
        if (fd <= 0) {
            LOGE("open /sys/class/video/frame_height  ERROR!!error = -%s- \n", strerror ( errno ));
            return -1;
        }
        memset(s, 0, sizeof(s));
        read(fd, s, sizeof(s));
        close(fd);
        nodeVal = atoi(s);

        if (nodeVal <= 576) {
            curFmtType = 1;
            sig_fmt = TVIN_SIG_FMT_HDMI_720X480P_60HZ;
        } else if (nodeVal > 567 && nodeVal <= 1088) {
            curFmtType = 2;
            sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
        } else {
            curFmtType = 3;
            sig_fmt = TVIN_SIG_FMT_HDMI_3840_2160_00HZ;
        }

        if (curFmtType != preFmtType) {
            LOGD("adjustPQparameters: nodeVal = %d, sig_fmt = %d.", nodeVal, sig_fmt);
            ret = myVpp->LoadVppSettings (mAutoPQSource, sig_fmt, _3d_type, trans_fmt);
        }

        preFmtType = curFmtType;

    } else {
        if (preFmtType != 0 || curFmtType != 0) {
            preFmtType = 0;
            curFmtType = 0;
        }
    }
    return ret;
}

bool CAutoPQparam::threadLoop()
{
    int sleeptime = 1000;//ms
    while ( mAutoPQ_OnOff_Flag ) {
        usleep ( sleeptime * 1000 );
        adjustPQparameters();
    }

    return false;//return true, run again, return false,not run.
}
