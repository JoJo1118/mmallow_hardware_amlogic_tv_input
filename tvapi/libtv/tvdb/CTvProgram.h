//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTvProgram.h
//  @ Date : 2013-11
//  @ Author :
//
//

#if !defined(_CTVPROGRAM_H)
#define _CTVPROGRAM_H

#include "CTvDatabase.h"
#include "CTvChannel.h"
#include <utils/String8.h>
#include <utils/RefBase.h>
#include <stdlib.h>
#include "CTvLog.h"
using namespace android;
// Program��ӦATV�е�һ��Ƶ����DTV�е�һ��service
class CTvEvent;
class CTvProgram : public LightRefBase<CTvProgram> {
public:
    /*this type value is link to  enum AM_SCAN_ServiceType in am_scan.h
     * enum AM_SCAN_ServiceType
    {
        AM_SCAN_SRV_UNKNOWN     = 0,    < δ֪����
        AM_SCAN_SRV_DTV         = 1,    < ���ֵ�������
        AM_SCAN_SRV_DRADIO      = 2,    < ���ֹ㲥����
        AM_SCAN_SRV_ATV         = 3,    < ģ���������
    };
    */
    /**δ��������*/
    static const int TYPE_UNKNOWN = 0;
    /**���ӽ�Ŀ*/
    static const int TYPE_TV    =  4;
    /**�㲥��Ŀ*/
    static const int TYPE_RADIO =  2;
    /**ģ���Ŀ*/
    static const int TYPE_ATV   =  3;
    /**���ݽ�Ŀ*/
    static const int TYPE_DATA  =  5;
    /**���ֽ�Ŀ*/
    static const int TYPE_DTV   = 1 ;
    /** PVR/Timeshifting playback program*/
    static const int TYPE_PLAYBACK =  6;

    static const int PROGRAM_SKIP_NO = 0;
    static const int PROGRAM_SKIP_YES = 1;
    static const int PROGRAM_SKIP_UNKOWN = 2;

    /**
     *Service�еĻ���Ԫ����Ϣ
     */
public:
    class Element {
    private :
        int mpid;

    public :
        Element(int pid)
        {
            this->mpid = pid;
        }
        /**
         *ȡ�û���Ԫ�ص�PID
         *@return ����PID
         */
        int getPID()
        {
            return mpid;
        }
    };



    /**
     *�����Ի���Ԫ����Ϣ
     */
public:
    class MultiLangElement : public Element {
    private :
        String8 mlang;

    public :
        MultiLangElement(int pid, String8 lang): Element(pid)
        {
            this->mlang = lang;
        }

        /**
         *ȡ��Ԫ�ض�Ӧ������
         *@return ����3�ַ������ַ���
         */
        String8 getLang()
        {
            return mlang;
        }
    };



    /**
    *��Ƶ��Ϣ
    */
public :
    class Video : public Element {
    public:
        /**MPEG1/2*/
        static const int FORMAT_MPEG12 = 0;
        /**MPEG4*/
        static const int FORMAT_MPEG4  = 1;
        /**H.264*/
        static const int FORMAT_H264   = 2;
        /**MJPEG*/
        static const int FORMAT_MJPEG  = 3;
        /**Real video*/
        static const int FORMAT_REAL   = 4;
        /**JPEG*/
        static const int FORMAT_JPEG   = 5;
        /**Microsoft VC1*/
        static const int FORMAT_VC1    = 6;
        /**AVS*/
        static const int FORMAT_AVS    = 7;
        /**YUV*/
        static const int FORMAT_YUV    = 8;
        /**H.264 MVC*/
        static const int FORMAT_H264MVC = 9;
        /**QJPEG*/
        static const int FORMAT_QJPEG  = 10;

        Video(int pid, int fmt): Element(pid)
        {
            this->mformat = fmt;
        }

        /**
         *ȡ����Ƶ�����ʽ
         *@return ������Ƶ�����ʽ
         */
        int getFormat()
        {
            return mformat;
        }
    private :
        int mformat;
    };

    /**
     *��Ƶ��Ϣ
     */
public :
    class Audio : public MultiLangElement {
    public  :
        /**MPEG*/
        static const int FORMAT_MPEG      = 0;
        /**PCM 16λС��*/
        static const int FORMAT_PCM_S16LE = 1;
        /**AAC*/
        static const int FORMAT_AAC       = 2;
        /**AC3*/
        static const int FORMAT_AC3       = 3;
        /**ALAW*/
        static const int FORMAT_ALAW      = 4;
        /**MULAW*/
        static const int FORMAT_MULAW     = 5;
        /**DTS*/
        static const int FORMAT_DTS       = 6;
        /**PCM 16λ���*/
        static const int FORMAT_PCM_S16BE = 7;
        /**FLAC*/
        static const int FORMAT_FLAC      = 8;
        /**COOK*/
        static const int FORMAT_COOK      = 9;
        /**PCM 8λ*/
        static const int FORMAT_PCM_U8    = 10;
        /**ADPCM*/
        static const int FORMAT_ADPCM     = 11;
        /**AMR*/
        static const int FORMAT_AMR       = 12;
        /**RAAC*/
        static const int FORMAT_RAAC      = 13;
        /**WMA*/
        static const int FORMAT_WMA       = 14;
        /**WMA Pro*/
        static const int FORMAT_WMAPRO    = 15;
        /**����PCM*/
        static const int FORMAT_PCM_BLURAY = 16;
        /**ALAC*/
        static const int FORMAT_ALAC      = 17;
        /**Vorbis*/
        static const int FORMAT_VORBIS    = 18;
        /**AAC latm��ʽ*/
        static const int FORMAT_AAC_LATM  = 19;
        /**APE*/
        static const int FORMAT_APE       = 20;


        Audio(int pid, String8 lang, int fmt): MultiLangElement(pid, lang)
        {
            this->mformat = fmt;
        }

        /**
         *ȡ����Ƶ�����ʽ
         *@return ������Ƶ�����ʽ
         */
        int getFormat()
        {
            return mformat;
        }
    private :
        int mformat;
    };

    /**
     *��Ļ��Ϣ
     */
public :
    class Subtitle : public MultiLangElement {
    public :
        /**DVB subtitle*/
        static const int TYPE_DVB_SUBTITLE = 1;
        /**���ֵ���Teletext*/
        static const int TYPE_DTV_TELETEXT = 2;
        /**ģ�����Teletext*/
        static const int TYPE_ATV_TELETEXT = 3;
        /**���ֵ���Closed caption*/
        static const int TYPE_DTV_CC = 4;
        /**ģ�����Closed caption*/
        static const int TYPE_ATV_CC = 5;



        Subtitle(int pid, String8 lang, int type, int num1, int num2): MultiLangElement(pid, lang)
        {

            this->type = type;
            if (type == TYPE_DVB_SUBTITLE) {
                compositionPage = num1;
                ancillaryPage   = num2;
            } else if (type == TYPE_DTV_TELETEXT) {
                magazineNo = num1;
                pageNo = num2;
            }
        }

        /**
         *ȡ����Ļ����
         *@return ������Ļ����
         */
        int getType()
        {
            return type;
        }

        /**
         *ȡ��DVB subtitle��composition page ID
         *@return ����composition page ID
         */
        int getCompositionPageID()
        {
            return compositionPage;
        }

        /**
         *ȡ��DVB subtitle��ancillary page ID
         *@return ����ancillary page ID
         */
        int getAncillaryPageID()
        {
            return ancillaryPage;
        }

        /**
         *ȡ��teletext��magazine number
         *@return ����magazine number
         */
        int getMagazineNumber()
        {
            return magazineNo;
        }

        /**
         *ȡ��teletext��page number
         *@return ����page number
         */
        int getPageNumber()
        {
            return pageNo;
        }

    private :
        int compositionPage;
        int ancillaryPage;
        int magazineNo;
        int pageNo;
        int type;
    };

    /**
     *Teletext��Ϣ
     */
public :
    class Teletext : public MultiLangElement {
    public:
        Teletext(int pid, String8 lang, int mag, int page): MultiLangElement(pid, lang)
        {
            magazineNo = mag;
            pageNo = page;
        }

        /**
         *ȡ��teletext��magazine number
         *@return ����magazine number
         */
        int getMagazineNumber()
        {
            return magazineNo;
        }

        /**
         *ȡ��teletext��page number
         *@return ����page number
         */
        int getPageNumber()
        {
            return pageNo;
        }

    private :
        int magazineNo;
        int pageNo;
    };

    //��Ŀ����Ϣ
public:
    /**���û�з�����Ƶ���������û�������*/
    static const int MINOR_CHECK_NONE         = 0;
    /**���û�з�����Ƶ��������Ѱ�ң���Ƶ���������ӣ�,�ҵ���Ƶ��������*/
    static const int MINOR_CHECK_UP           = 1;
    /**���û�з�����Ƶ��������Ѱ�ң���Ƶ�����ּ�С��,�ҵ���Ƶ������С��*/
    static const int MINOR_CHECK_DOWN         = 2;
    /*���û�з�����Ƶ��������Ѱ�ң�Ȼ���ҵ����������.*/
    static const int MINOR_CHECK_NEAREST_UP   = 3;
    /*���û�з�����Ƶ��������Ѱ�ң�Ȼ���ҵ����������.*/
    static const int MINOR_CHECK_NEAREST_DOWN = 4;

    /**
     *ȡ�ý�Ŀ��
     *@return ���ؽ�Ŀ��
     */
    int getNumber()
    {
        return major;
    }

    /**
     *ȡ������Ŀ��(ATSC)
     *@return ���ؽ�Ŀ������Ŀ��
     */
    int getMajor()
    {
        return major;
    }

    /**
     *ȡ�ôν�Ŀ��(ATSC)
     *@return ���ؽ�Ŀ�Ĵν�Ŀ��
     */
    int getMinor()
    {
        return minor;
    }

    /**
     *�Ƿ�ΪATSCģʽ
     *@return �����ATSCģʽ����true
     */
    bool isATSCMode()
    {
        return atscMode;
    }

    /**
     *ȡ����Ƶ�����Զ����Ҳ���(ATSC)
     *@return ������Ƶ�����Զ����Ҳ���
     */
    int getMinorCheck()
    {
        return minorCheck;
    }

private:
    int major;
    int minor;
    int minorCheck;
    bool atscMode;


public:
    CTvProgram(CTvDatabase::Cursor &c);
    CTvProgram(int channelID, int type, int num, int skipFlag);
    /**
     *�����ݿ����һ��Program,atscMode
    */
    CTvProgram(int channelID, int type, int major, int minor, int skipFlag);
    ~CTvProgram();
    // �����������ݿ����һ��Program
    CTvProgram(int channelID, int type);

    CTvProgram();


    int getCurrentAudio(String8 defaultLang);
    Video *getVideo()
    {
        return mpVideo;
    }
    Audio *getAudio(int id)
    {
        if (mvAudios.size() <= 0) return NULL;
        return mvAudios[id];
    }

    int getAudioTrackSize()
    {
        return mvAudios.size();
    }
    static int selectByID(int id, CTvProgram &p);
    static CTvProgram selectByNumber(int num, int type);
    int selectByNumber(int type, int major, int minor, CTvProgram &prog, int minor_check = MINOR_CHECK_NONE);
    static int selectByNumber(int type, int num, CTvProgram &prog);
    static int selectByChannel(int channelID, int type, Vector<sp<CTvProgram> > &out);
    // �г�ȫ��TVProgram
    static int selectAll(bool no_skip, Vector<sp<CTvProgram> > &out);
    static int selectByType(int type, int skip, Vector<sp<CTvProgram> > &out);
    static int selectByChanID(int type, int skip, Vector<sp<CTvProgram> > &out);
    static Vector<CTvProgram> selectByChannel(int channelID);
    // ���ݽ�Ŀ�����еĹؼ��ֲ���ָ��TVProgram
    static Vector<CTvProgram>  selectByName(int name);
    void tvProgramDelByChannelID(int channelID);
    int getID()
    {
        return id;
    };
    int getSrc()
    {
        return src;
    };
    int getProgType()
    {
        return type;
    };
    int getChanOrderNum()
    {
        return chanOrderNum;
    };
    int getChanVolume()
    {
        return volume;
    };
    int getSourceId()
    {
        return sourceID;
    };
    int getServiceId()
    {
        return dvbServiceID;
    };
    int getProgSkipFlag();
    int getSubtitleIndex(int progId);
    int setSubtitleIndex(int progId, int index);
    void setCurrAudioTrackIndex(int programId, int audioIndex);
    int getCurrAudioTrackIndex();

    String8 getName();
    void getCurrentSubtitle();
    void getCurrentTeletext();
    int getChannel(CTvChannel &c);
    int upDateChannel(CTvChannel &c, int std, int freq, int fineFreq);
    int updateVolComp(int progID, int volValue);
    void updateProgramName(int progId, String8 strName);
    void setSkipFlag(int progId, bool bSkipFlag);
    void setFavoriteFlag(int progId, bool bFavor);
    int getFavoriteFlag()
    {
        return favorite;
    };
    void deleteProgram(int progId);
    static int CleanAllProgramBySrvType(int srvType);
    void setLockFlag(int progId, bool bLockFlag);
    bool getLockFlag();
    void swapChanOrder(int ProgId1, int chanOrderNum1, int ProgId2, int chanOrderNum2);
    int getAudioChannel();
    static int updateAudioChannel(int progId, int ch);
    static int deleteChannelsProgram(CTvChannel &c);
    Vector<Subtitle *> getSubtitles()
    {
        return mvSubtitles;
    }
private:
    int CreateFromCursor(CTvDatabase::Cursor &c);
    int selectProgramInChannelByNumber(int channelID, int num, CTvDatabase::Cursor &c);
    int selectProgramInChannelByNumber(int channelID, int major, int minor, CTvDatabase::Cursor &c);
    CTvChannel channel;
    int id;
    int dvbServiceID;
    int type;
    String8 name;
    int channelID;
    int skip;
    int favorite;
    int volume;
    int sourceID;
    int pmtPID;
    int src;
    int audioTrack;
    int chanOrderNum;
    int currAudTrackIndex;
    bool lock;
    bool scrambled;
    // video��Ϣ,���Ͳ���
    Video *mpVideo;
    // audio��Ϣ,���Ͳ���
    Vector<Audio *> mvAudios;
    // subtitle��Ϣ���Ͳ���
    Vector<Subtitle *> mvSubtitles;
    // teletext��Ϣ,���Ͳ���
    Vector<Teletext *> mvTeletexts;

};

#endif  //_CTVPROGRAM_H
