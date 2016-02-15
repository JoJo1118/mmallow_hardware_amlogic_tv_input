//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTvChannel.h
//  @ Date : 2013-11
//  @ Author :
//
//


#if !defined(_CTVCHANNEL_H)
#define _CTVCHANNEL_H
#include <utils/Vector.h>
#include "CTvDatabase.h"
#include <utils/String8.h>
#include <utils/RefBase.h>
#include <stdlib.h>

#include "CTvLog.h"
using namespace android;
// ��Ӧģ������е�һ��Ƶ�㣬���ֵ����е�һ��Ƶ����Ƶ�TS��
class CTvChannel: public LightRefBase<CTvChannel> {

public :
    static const int FE_HAS_SIGNAL   = 0x01;
    static const int FE_HAS_CARRIER  = 0x02;
    static const int FE_HAS_VITERBI  = 0x04;
    static const int FE_HAS_SYNC     = 0x08;
    /**����*/
    static const int FE_HAS_LOCK     = 0x10;
    /**��ʱ*/
    static const int FE_TIMEDOUT     = 0x20;
    static const int FE_REINIT       = 0x40;

    /**QPSKģʽ*/
    static const int MODE_QPSK = 0;
    /**QAMģʽ*/
    static const int MODE_QAM  = 1;
    /**OFDMģʽ*/
    static const int MODE_OFDM = 2;
    /**ATSCģʽ*/
    static const int MODE_ATSC = 3;
    /**ģ��ģʽ*/
    static const int MODE_ANALOG = 4;
    /**DTMBģʽ*/
    static const int MODE_DTMB = 5;


    /**8MHz����*/
    static const int BANDWIDTH_8_MHZ = 0;
    /**7MHz����*/
    static const int BANDWIDTH_7_MHZ = 1;
    /**6MHz����*/
    static const int BANDWIDTH_6_MHZ = 2;
    /**�Զ�������*/
    static const int BANDWIDTH_AUTO  = 3;
    /**5MHZ����*/
    static const int BANDWIDTH_5_MHZ = 4;
    /**10MHZ����*/
    static const int BANDWIDTH_10_MHZ = 5;

    /**QPSK����*/
    static const int MODULATION_QPSK    = 0;
    /**QAM16����*/
    static const int MODULATION_QAM_16  = 1;
    /**QAM32����*/
    static const int MODULATION_QAM_32  = 2;
    /**QAM64����*/
    static const int MODULATION_QAM_64  = 3;
    /**QAM128����*/
    static const int MODULATION_QAM_128 = 4;
    /**QAM256����*/
    static const int MODULATION_QAM_256 = 5;
    /**QAM����(�Զ����)*/
    static const int MODULATION_QAM_AUTO = 6;
    /**VSB8����*/
    static const int MODULATION_VSB_8   = 7;
    /**VSB16����*/
    static const int MODULATION_VSB_16  = 8;
    /**PSK8����*/
    static const int MODULATION_PSK_8   = 9;
    /**APSK16����*/
    static const int MODULATION_APSK_16 = 10;
    /**APSK32����*/
    static const int MODULATION_APSK_32 = 11;
    /**DQPSK����*/
    static const int MODULATION_DQPSK   = 12;

    /**������*/
    static const int AUDIO_MONO   = 0x0000;
    /**������*/
    static const int AUDIO_STEREO = 0x0001;
    /**����2*/
    static const int AUDIO_LANG2  = 0x0002;
    /**SAP*/
    static const int AUDIO_SAP    = 0x0002;
    /**����1*/
    static const int AUDIO_LANG1  = 0x0003;
    /**����1/2*/
    static const int AUDIO_LANG1_LANG2 = 0x0004;

    /**PAL B*/
    static const int STD_PAL_B     = 0x00000001;
    /**PAL B1*/
    static const int STD_PAL_B1    = 0x00000002;
    /**PAL G*/
    static const int STD_PAL_G     = 0x00000004;
    /**PAL H*/
    static const int STD_PAL_H     = 0x00000008;
    /**PAL I*/
    static const int STD_PAL_I     = 0x00000010;
    /**PAL D*/
    static const int STD_PAL_D     = 0x00000020;
    /**PAL D1*/
    static const int STD_PAL_D1    = 0x00000040;
    /**PAL K*/
    static const int STD_PAL_K     = 0x00000080;
    /**PAL M*/
    static const int STD_PAL_M     = 0x00000100;
    /**PAL N*/
    static const int STD_PAL_N     = 0x00000200;
    /**PAL Nc*/
    static const int STD_PAL_Nc    = 0x00000400;
    /**PAL 60*/
    static const int STD_PAL_60    = 0x00000800;
    /**NTSC M*/
    static const int STD_NTSC_M    = 0x00001000;
    /**NTSC M JP*/
    static const int STD_NTSC_M_JP = 0x00002000;
    /**NTSC 443*/
    static const int STD_NTSC_443  = 0x00004000;
    /**NTSC M KR*/
    static const int STD_NTSC_M_KR = 0x00008000;
    /**SECAM B*/
    static const int STD_SECAM_B   = 0x00010000;
    /**SECAM D*/
    static const int STD_SECAM_D   = 0x00020000;
    /**SECAM G*/
    static const int STD_SECAM_G   = 0x00040000;
    /**SECAM H*/
    static const int STD_SECAM_H   = 0x00080000;
    /**SECAM K*/
    static const int STD_SECAM_K   = 0x00100000;
    /**SECAM K1*/
    static const int STD_SECAM_K1  = 0x00200000;
    /**SECAM L*/
    static const int STD_SECAM_L   = 0x00400000;
    /**SECAM LC*/
    static const int STD_SECAM_LC  = 0x00800000;
    /**ATSC VSB8*/
    static const int STD_ATSC_8_VSB  = 0x01000000;
    /**ATSC VSB16*/
    static const int STD_ATSC_16_VSB = 0x02000000;
    /**NTSC*/
    static const int STD_NTSC      = STD_NTSC_M | STD_NTSC_M_JP | STD_NTSC_M_KR;
    /**SECAM DK*/
    static const int STD_SECAM_DK  = STD_SECAM_D | STD_SECAM_K | STD_SECAM_K1;
    /**SECAM*/
    static const int STD_SECAM     = STD_SECAM_B | STD_SECAM_G | STD_SECAM_H | STD_SECAM_DK | STD_SECAM_L | STD_SECAM_LC;
    /**PAL BG*/
    static const int STD_PAL_BG    = STD_PAL_B | STD_PAL_B1 | STD_PAL_G;
    /**PAL DK*/
    static const int STD_PAL_DK    = STD_PAL_D | STD_PAL_D1 | STD_PAL_K;
    /**PAL*/
    static const int STD_PAL       = STD_PAL_BG | STD_PAL_DK | STD_PAL_H | STD_PAL_I;

    //static const int TUNER_STD_MN  = STD_PAL_M|STD_PAL_N|STD_PAL_Nc| STD_NTSC
    static const int STD_B         = STD_PAL_B    | STD_PAL_B1   | STD_SECAM_B;
    static const int STD_GH        = STD_PAL_G    | STD_PAL_H   | STD_SECAM_G  | STD_SECAM_H;
    static const int STD_DK        = STD_PAL_DK   | STD_SECAM_DK;
    static const int STD_M         = STD_PAL_M    | STD_NTSC_M;
    static const int STD_BG        = STD_PAL_BG   | STD_SECAM_B | STD_SECAM_G ;

    static const int COLOR_AUTO  = 0x02000000;
    static const int COLOR_PAL   = 0x04000000;
    static const int COLOR_NTSC  = 0x08000000;
    static const int COLOR_SECAM = 0x10000000;

    /**ˮƽ����*/
    static const int SAT_POLARISATION_H = 0;
    /**��ֱ����*/
    static const int SAT_POLARISATION_V = 1;

public:
    CTvChannel();
    CTvChannel(int dbID, int mode, int freq, int bw, int mod, int sym, int ofdm, int channelNum);
    ~CTvChannel();
    // ����TVChannelParams,�����ݿ����һ��Channel

    // *������ǰ��������ͨ��
    // *@param sat_id ����id
    // *@return ���ش�����ͨ��
    static Vector<CTvChannel> tvChannelList(int sat_id);
    static int selectByID(int id, CTvChannel &c);
    static int updateByID(int progID, int std, int freq, int fineFreq);
    static  int SelectByFreq(int freq, CTvChannel &channel);
    static int DeleteBetweenFreq(int beginFreq, int endFreq);
    static int CleanAllChannelBySrc(int src);
    static int getChannelListBySrc(int src, Vector< sp<CTvChannel> > &v_channel);
    void tvChannelDel();
    static void tvChannelDelBySatID(int id);
    int  getID()
    {
        return id;
    };
    int getDVBTSID();
    void getDVBOrigNetID();
    void getFrontendID();
    void getTSSourceID();
    void isDVBCMode();
    void setFrequency();
    int getFrequency()
    {
        return frequency;
    }
    int getSymbolRate()
    {
        return symbolRate;
    }
    int getModulation()
    {
        return modulation;
    }
    int getBandwidth()
    {
        return bandwidth;
    }
    int getMode()
    {
        return mode;
    }

    int getStd()
    {
        return standard;
    };
    int getAfcData()
    {
        return afc_data;
    };
    int getLogicalChannelNum()
    {
        return logicalChannelNum;
    };
    // ���÷�����(QPSK/QAMģʽ)
    // @param symbolRate ������
    void setSymbolRate();
    // ���ü���(QPSKģʽ)
    void setPolarisation();
    void setATVAudio();
    // �޸�ģ����Ƶ��ʽ
    void setATVVideoFormat();
    void setATVAudioFormat();
    void setATVFreq();
    void setATVAfcData();
    //
private:

    void createFromCursor(CTvDatabase::Cursor &c);

    //
    int id;
    int dvbTSID;
    int dvbOrigNetID;
    int fendID;
    int tsSourceID;

    int mode;
    int frequency;
    int symbolRate;
    int modulation;
    int bandwidth;
    int audio;
    int standard;
    int afc_data;
    int sat_id;
    int logicalChannelNum;
    //showboz
    //public TVSatelliteParams tv_satparams;
    int sat_polarisation;

};

#endif  //_CTVCHANNEL_H

