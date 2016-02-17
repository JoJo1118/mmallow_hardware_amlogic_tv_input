//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name :
//  @ Date : 2013-11
//  @ Author :
//
//
#include "CTvScanner.h"
#include "CTvProgram.h"
#include "CTvRegion.h"
#include "CFrontEnd.h"
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "CTvScanner"
#endif

CTvScanner *CTvScanner::m_s_Scanner = NULL;
//int CTvScanner::getStartPara()
//{
//
//}
CTvScanner::CTvScanner()
{
    mbScanStart = false;
    mpObserver = NULL;
    mSource = 0xff;
    mMinFreq = 1;
    mMaxFreq = 100;
    mCurScanStartFreq = 1;
    mCurScanEndFreq = 100;
}

CTvScanner::~CTvScanner()
{
    AM_EVT_Unsubscribe((long)mScanHandle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
    AM_EVT_Unsubscribe((long)mScanHandle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
}


AM_Bool_t CTvScanner::atv_cvbs_lock_check(v4l2_std_id  *colorStd)
{
    tvafe_cvbs_video_t cvbs_lock_status;
    int ret, i = 0;

    *colorStd = 0;
    while (i < 20) {
        ret = CTvin::getInstance()->AFE_GetCVBSLockStatus(&cvbs_lock_status);

        if (cvbs_lock_status == TVAFE_CVBS_VIDEO_HV_LOCKED)
            //||cvbs_lock_status == TVAFE_CVBS_VIDEO_V_LOCKED
            //||cvbs_lock_status == TVAFE_CVBS_VIDEO_H_LOCKED)
        {
            usleep(2000 * 1000);
            tvin_info_t info;
            CTvin::getInstance()->VDIN_GetSignalInfo(&info);
            *colorStd = CTvin::CvbsFtmToV4l2ColorStd(info.fmt);
            LOGD("atv_cvbs_lock_check     locked   and cvbs fmt = 0x%x std = 0x%x", info.fmt, *colorStd);
            return true;
        }
        usleep(50 * 1000);
        i++;
    }
    return false;
}

AM_Bool_t CTvScanner::s_atv_cvbs_lock_check(void *data)
{
    if (data == NULL) return false;
    AM_SCAN_ATV_LOCK_PARA_t *pAtvPara = (AM_SCAN_ATV_LOCK_PARA_t *)data;
    CTvScanner *pScan = (CTvScanner *)(pAtvPara->pData);
    v4l2_std_id std;
    AM_Bool_t isLock = pScan->atv_cvbs_lock_check(&std);
    pAtvPara->pOutColorSTD = std;
    return isLock;
}

int CTvScanner::ATVManualScan(int min_freq, int max_freq, int std, int store_Type, int channel_num)
{
    stopScan();
    if (min_freq <= 0 || max_freq <= 0 || min_freq == max_freq) {
        LOGD("atv manual scan freq error start = %d end = %d",  min_freq,  max_freq);
        return -1;
    }
    int direction = (max_freq >= min_freq) ? 1 : 0;

    if (!mbScanStart) { //not start or stop
        //
        mCurScanStartFreq = min_freq;
        mCurScanEndFreq = max_freq;
        AM_SCAN_CreatePara_t para;
        AM_DMX_OpenPara_t dmx_para;
        AM_SCAN_Handle_t handle = 0, i;
        LOGD("startScanThread");
        // Create the scan
        memset(&para, 0, sizeof(para));
        para.fend_dev_id = 0;//default
        para.mode = AM_SCAN_MODE_ATV_DTV;//
        para.atv_para.mode = AM_SCAN_ATVMODE_MANUAL;
        para.atv_para.storeMode = store_Type;
        para.dtv_para.mode = AM_SCAN_DTVMODE_NONE;

        para.atv_para.am_scan_atv_cvbs_lock =  s_atv_cvbs_lock_check;

        para.atv_para.fe_cnt = 3;
        para.atv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t *>(calloc(3, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)));
        if (para.atv_para.fe_paras != NULL) {
            memset(para.atv_para.fe_paras, 0, 3 * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));
            para.atv_para.fe_paras[0].m_type = FE_ANALOG;
            para.atv_para.fe_paras[0].analog.para.frequency = min_freq;
            para.atv_para.fe_paras[1].m_type = FE_ANALOG;
            para.atv_para.fe_paras[1].analog.para.frequency = max_freq;
            para.atv_para.fe_paras[2].m_type = FE_ANALOG;
            para.atv_para.fe_paras[2].analog.para.frequency = min_freq;
        }
        para.atv_para.direction = direction;
        para.atv_para.default_std = std;

        para.atv_para.channel_id = -1;
        para.atv_para.cvbs_unlocked_step = 1000000;
        para.atv_para.cvbs_locked_step = 3000000;
        para.atv_para.afc_range = 2000000;

        para.dtv_para.resort_all = AM_FALSE;
        para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
        para.store_cb = am_scan_atv_store;
        para.atv_para.channel_num = channel_num;

        memset(&dmx_para, 0, sizeof(dmx_para));
        AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

        if ((para.dtv_para.mode & 0x07) != AM_SCAN_DTVMODE_NONE) {
            AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
            tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
        }

        LOGD("atv manual scan mode = %d", para.dtv_para.mode);
        // Start Scan
        if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
            LOGD("SCAN CREATE fail");
            handle = NULL;
        } else {
            mScanHandle = handle;
            AM_SCAN_SetUserData(handle, (void *)this);
            AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
            AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
            if (AM_SCAN_Start(handle) != AM_SUCCESS) {
                AM_SCAN_Destroy(handle, AM_FALSE);
                AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
                AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
                handle = NULL;
            }
        }
        if (para.atv_para.fe_paras != NULL)
            free(para.atv_para.fe_paras);
        if (para.dtv_para.fe_paras != NULL)
            free(para.dtv_para.fe_paras);

        if (handle == NULL) {
            return -1;
        }
        mbScanStart = true;//start call ok
    } else {
        LOGD("scanning and not stop");
    }

    return 0;

}

void CTvScanner::am_scan_atv_store(AM_SCAN_Result_t *result)
{
    AM_SCAN_TS_t *ts;
    int i, ret, db_sat_id = -1;
    //AM_SCAN_RecTab_t srv_tab;
    for (ts = result->tses; ts != NULL; ts = ts->p_next) {
        //
        m_s_Scanner->mCurEv.clear();
        m_s_Scanner->mCurEv.mType = ScannerEvent::EVENT_ATV_PROG_DATA;
        m_s_Scanner->mCurEv.mVideoStd = CFrontEnd::stdAndColorToVideoEnum(ts->analog.std);
        m_s_Scanner->mCurEv.mAudioStd = CFrontEnd::stdAndColorToAudioEnum(ts->analog.std);
        m_s_Scanner->mCurEv.mIsAutoStd = ((ts->analog.std  &  V4L2_COLOR_STD_AUTO) ==  V4L2_COLOR_STD_AUTO) ? 1 : 0;
        m_s_Scanner->mCurEv.mFrequency = ts->analog.freq;
        //
        m_s_Scanner->sendEvent(m_s_Scanner->mCurEv);
        LOGD("ATV: > freq:%d", m_s_Scanner->mCurEv.mFrequency);
    }
}

void CTvScanner::setGlobalScanerObject(CTvScanner *s)
{
    m_s_Scanner =  s;
}

int CTvScanner::autoAtvScan(int min_freq, int max_freq, int std, int search_type)
{
    stopScan();
    if (min_freq <= 0 || max_freq <= 0 || min_freq > max_freq)
        return -1;
    if (!mbScanStart) { //not start or stop
        mCurScanStartFreq = min_freq;
        mCurScanEndFreq = max_freq;

        AM_SCAN_CreatePara_t para;
        AM_DMX_OpenPara_t dmx_para;
        AM_SCAN_Handle_t handle = 0, i;
        // Create the scan
        memset(&para, 0, sizeof(para));

        para.fend_dev_id = 0;//default
        para.mode = AM_SCAN_MODE_ATV_DTV;//
        para.atv_para.mode = AM_SCAN_ATVMODE_AUTO;
        if (search_type == 1)
            para.atv_para.storeMode = 1;
        else
            para.atv_para.storeMode = 0;
        para.dtv_para.mode = AM_SCAN_DTVMODE_NONE;

        para.atv_para.am_scan_atv_cvbs_lock =  s_atv_cvbs_lock_check;

        para.atv_para.fe_cnt = 3;
        para.atv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t *>(calloc(3, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)));
        if (para.atv_para.fe_paras != NULL) {
            memset(para.atv_para.fe_paras, 0, 3 * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));
            para.atv_para.fe_paras[0].m_type = FE_ANALOG;
            para.atv_para.fe_paras[0].analog.para.frequency = min_freq;
            para.atv_para.fe_paras[1].m_type = FE_ANALOG;
            para.atv_para.fe_paras[1].analog.para.frequency = max_freq;
            para.atv_para.fe_paras[2].m_type = FE_ANALOG;
            para.atv_para.fe_paras[2].analog.para.frequency = 0;
        }

        para.atv_para.direction = 0;
        para.atv_para.default_std = std;

        para.atv_para.channel_id = -1;
        para.atv_para.afc_unlocked_step = 3000000;
        para.atv_para.cvbs_unlocked_step = 1500000;
        para.atv_para.cvbs_locked_step = 6000000;
        para.atv_para.afc_range = 2000000;

        para.dtv_para.resort_all = AM_FALSE;
        para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
        para.store_cb = am_scan_atv_store;

        memset(&dmx_para, 0, sizeof(dmx_para));
        AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

        if ((para.dtv_para.mode & 0x07) != AM_SCAN_DTVMODE_NONE) {
            AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
            tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
        }

        //    prog->dmx_id = para.dtv_para.dmx_dev_id;
        //    prog->fend_id = para.fend_dev_id;
        //    prog->mode = para.dtv_para.mode;
        LOGD(" mode = %d", para.dtv_para.mode);
        // Start Scan
        if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
            LOGD("SCAN CREATE fail");
            handle = NULL;
        } else {
            mScanHandle = handle;
            AM_SCAN_SetUserData(handle, (void *)this);
            //注册搜索事件
            AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
            //注册信号质量通知事件
            AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
            if (AM_SCAN_Start(handle) != AM_SUCCESS) {
                AM_SCAN_Destroy(handle, AM_FALSE);
                AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
                AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
                handle = NULL;
            }
        }
        if (para.atv_para.fe_paras != NULL)
            free(para.atv_para.fe_paras);
        if (para.dtv_para.fe_paras != NULL)
            free(para.dtv_para.fe_paras);

        if (handle == NULL) {
            return -1;
        }
        mbScanStart = true;//start call ok
    } else {
        LOGD("scanning and not stop");
    }

    return 0;
}

/*
int CTvScanner::manualAtscScan(int channelID, int attennaType, int std)
{
	stopScan();
	if (!mbScanStart) { //not start or stop
		AM_SCAN_CreatePara_t para;
		AM_DMX_OpenPara_t dmx_para;
		AM_SCAN_Handle_t handle = 0;
		int i;
		// Create the scan
		memset(&para, 0, sizeof(para));

		//strcpy(para.default_text_lang,"eng");//config
		//strcpy(para.text_langs, "local eng zho chi chs first");//config
		para.fend_dev_id = 0;//default
		para.mode = AM_SCAN_MODE_ADTV;//DTV
		para.atv_para.mode = AM_SCAN_ATVMODE_NONE;

		para.atv_para.default_std = std;

		para.dtv_para.mode = AM_SCAN_DTVMODE_MANUAL;//DTV_MODE_ALLBAND
		//para.dtv_para.mode |= (*env)->GetIntField(env, para, doptions);//忽略
		para.dtv_para.source = FE_ATSC;//fe_type
		para.dtv_para.dmx_dev_id = 0;//default 0
		para.dtv_para.resort_all = AM_TRUE;
		para.dtv_para.clear_source = AM_FALSE;
		para.dtv_para.mix_tv_radio = AM_FALSE;
		para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
		para.store_cb = NULL;
		//para.dtv_para.attenna = attennaType;

		para.dtv_para.standard = AM_SCAN_DTV_STD_ATSC;

		mSource = para.dtv_para.source;

		Vector<sp<CTvChannel> > vcp;

		//CTvDatabase::getChannelParaList("/data/tv_default.xml", vcp);//channel list from xml or db
		getAtscChannelPara(attennaType, vcp);
		LOGD("ANDESEN test attenna type is %d", attennaType);

		int size = vcp.size();
		LOGD("channel list size = %d", size);
		//showboz
		if (size == 0) {
			CTvDatabase::GetTvDb()->importXmlToDB("/etc/tv_default.xml");
			getAtscChannelPara(attennaType, vcp);
			size = vcp.size();
		}

		if (!(para.dtv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t *>(calloc(1, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
			return -1;

		//memset(pfp, 0, size * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));

		for (i = 0; i < size; i++) {
			if (channelID == vcp[i]->getLogicalChannelNum()) {
				para.dtv_para.fe_paras[0].m_type = 3;//MODE_ATSC
				para.dtv_para.fe_paras[0].atsc.para.frequency = vcp[i]->getFrequency();
				para.dtv_para.fe_paras[0].atsc.para.u.vsb.modulation = (fe_modulation_t)(vcp[i]->getModulation());
				LOGD("andesen test atsc manual type=%d,num=%d,freq=%d,modu=%d",
					 para.dtv_para.fe_paras[0].m_type, para.dtv_para.fe_paras[0].m_logicalChannelNum,
					 para.dtv_para.fe_paras[0].atsc.para.frequency, para.dtv_para.fe_paras[0].atsc.para.u.vsb.modulation);
			}
		}
		//allband
		para.dtv_para.fe_cnt = 1;
		memset(&dmx_para, 0, sizeof(dmx_para));
		AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

		if ((para.dtv_para.mode & 0x07) != AM_SCAN_DTVMODE_NONE) {
			AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
			tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
		}

		//    prog->dmx_id = para.dtv_para.dmx_dev_id;
		//    prog->fend_id = para.fend_dev_id;
		//    prog->mode = para.dtv_para.mode;
		// Start Scan
		if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
			LOGD("SCAN CREATE fail");
			handle = 0;
		} else {
			mScanHandle = handle;
			AM_SCAN_SetUserData(handle, (void *)this);
			//注册搜索事件
			AM_EVT_Subscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
			//注册信号质量通知事件
			AM_EVT_Subscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
			if (AM_SCAN_Start(handle) != AM_SUCCESS) {
				AM_SCAN_Destroy(handle, AM_FALSE);
				AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
				AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
				handle = 0;
			}
		}
		if (para.atv_para.fe_paras != NULL)
			free(para.atv_para.fe_paras);
		if (para.dtv_para.fe_paras != NULL)
			free(para.dtv_para.fe_paras);
		if (handle == 0) {
			return -1;
		}
		mbScanStart = true;//start call ok
	} else {
		LOGD("scanning and not stop");
	}
	return 0;
}
*/

#define dvb_fend_para(_p) ((struct dvb_frontend_parameters*)(&_p))
#define IS_DVBT2_TS(_para) (_para.m_type == FE_OFDM && _para.terrestrial.para.u.ofdm.ofdm_mode == OFDM_DVBT2)
#define IS_ISDBT_TS(_para) (_para.m_type == FE_ISDBT)

dvbpsi_pat_t *CTvScanner::get_valid_pats(AM_SCAN_TS_t *ts)
{
    dvbpsi_pat_t *valid_pat = NULL;
    if (!IS_DVBT2_TS(ts->digital.fend_para)) {
        valid_pat = ts->digital.pats;
    } else if (IS_ISDBT_TS(ts->digital.fend_para)) {
        /* process for isdbt one-seg inserted PAT, which ts_id is 0xffff */
        valid_pat = ts->digital.pats;
        while (valid_pat != NULL && valid_pat->i_ts_id == 0xffff) {
            valid_pat = valid_pat->p_next;
        }

        if (valid_pat == NULL && ts->digital.pats != NULL) {
            valid_pat = ts->digital.pats;

            if (ts->digital.sdts != NULL)
                valid_pat->i_ts_id = ts->digital.sdts->i_ts_id;
        }
    } else {
        int plp;

        for (plp = 0; plp < ts->digital.dvbt2_data_plp_num; plp++) {
            if (ts->digital.dvbt2_data_plps[plp].pats != NULL) {
                valid_pat = ts->digital.dvbt2_data_plps[plp].pats;
                break;
            }
        }
    }

    return valid_pat;
}

void CTvScanner::scan_process_ts_info(AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, ScannerEvent *evt)
{
    dvbpsi_nit_t *nit;
    dvbpsi_descriptor_t *descr;

    evt->mONetId = -1;
    evt->mTsId = -1;

    if (ts->digital.sdts)
        evt->mONetId = ts->digital.sdts->i_network_id;
    else if (IS_DVBT2_TS(ts->digital.fend_para) && ts->digital.dvbt2_data_plp_num > 0 && ts->digital.dvbt2_data_plps[0].sdts)
        evt->mONetId = ts->digital.dvbt2_data_plps[0].sdts->i_network_id;
    evt->mTsId = get_valid_pats(ts)->i_ts_id;

    evt->mFrequency = (int)dvb_fend_para(ts->digital.fend_para)->frequency;
    evt->mMode = ts->digital.fend_para.m_type;
    if (IS_DVBT2_TS(ts->digital.fend_para))
        evt->mOfdm_mode = (int)dvb_fend_para(ts->digital.fend_para)->u.ofdm.ofdm_mode;
    else if ((ts->digital.fend_para.m_type == FE_DTMB) || (ts->digital.fend_para.m_type == FE_OFDM))
        evt->mBandwidth = (int)dvb_fend_para(ts->digital.fend_para)->u.ofdm.bandwidth;
    else if (ts->digital.fend_para.m_type == FE_QAM) {
        evt->mSymbolRate = (int)dvb_fend_para(ts->digital.fend_para)->u.qam.symbol_rate;
        evt->mModulation = (int)dvb_fend_para(ts->digital.fend_para)->u.qam.modulation;
    }
}
void CTvScanner::scan_init_service_info(SCAN_ServiceInfo_t *srv_info)
{
    memset(srv_info, 0, sizeof(SCAN_ServiceInfo_t));
    srv_info->vid = 0x1fff;
    srv_info->vfmt = -1;
    srv_info->free_ca = 1;
    srv_info->srv_id = 0xffff;
    srv_info->srv_dbid = -1;
    srv_info->satpara_dbid = -1;
    srv_info->pmt_pid = 0x1fff;
    srv_info->plp_id = -1;
    srv_info->sdt_version = 0xff;
}
int CTvScanner::get_pmt_pid(dvbpsi_pat_t *pats, int program_number)
{
    dvbpsi_pat_t *pat;
    dvbpsi_pat_program_t *prog;

    AM_SI_LIST_BEGIN(pats, pat)
    AM_SI_LIST_BEGIN(pat->p_first_program, prog)
    if (prog->i_number == program_number)
        return prog->i_pid;
    AM_SI_LIST_END()
    AM_SI_LIST_END()

    return 0x1fff;
}

void CTvScanner::scan_extract_ca_scrambled_flag(dvbpsi_descriptor_t *p_first_descriptor, int *flag)
{
    dvbpsi_descriptor_t *descr;

    AM_SI_LIST_BEGIN(p_first_descriptor, descr)
    if (descr->i_tag == AM_SI_DESCR_CA && ! *flag) {
        LOGD( "Found CA descr, set scrambled flag to 1");
        *flag = 1;
        break;
    }
    AM_SI_LIST_END()
}

void CTvScanner::scan_extract_srv_info_from_sdt(AM_SCAN_Result_t *result, dvbpsi_sdt_t *sdts, SCAN_ServiceInfo_t *srv_info)
{
    dvbpsi_sdt_service_t *srv;
    dvbpsi_sdt_t *sdt;
    dvbpsi_descriptor_t *descr;
    const uint8_t split = 0x80;
    const int name_size = (int)sizeof(srv_info->name);
    int curr_name_len = 0, tmp_len;
    char name[AM_DB_MAX_SRV_NAME_LEN + 1];

    UNUSED(result);

#define COPY_NAME(_s, _slen)\
    AM_MACRO_BEGIN\
    int copy_len = ((curr_name_len+_slen)>=name_size) ? (name_size-curr_name_len) : _slen;\
        if (copy_len > 0) {\
            memcpy(srv_info->name+curr_name_len, _s, copy_len);\
            curr_name_len += copy_len;\
        }\
    AM_MACRO_END


    AM_SI_LIST_BEGIN(sdts, sdt)
    AM_SI_LIST_BEGIN(sdt->p_first_service, srv)
    /*从SDT表中查找该service并获取信息*/
    if (srv->i_service_id == srv_info->srv_id) {
        LOGD("SDT for service %d found!", srv_info->srv_id);
        srv_info->eit_sche = (uint8_t)srv->b_eit_schedule;
        srv_info->eit_pf = (uint8_t)srv->b_eit_present;
        srv_info->rs = srv->i_running_status;
        srv_info->free_ca = (uint8_t)srv->b_free_ca;
        srv_info->sdt_version = sdt->i_version;

        AM_SI_LIST_BEGIN(srv->p_first_descriptor, descr)
        if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_SERVICE) {
            dvbpsi_service_dr_t *psd = (dvbpsi_service_dr_t *)descr->p_decoded;
            if (psd->i_service_name_length > 0) {
                name[0] = 0;
                AM_SI_ConvertDVBTextCode((char *)psd->i_service_name, psd->i_service_name_length, \
                                         name, AM_DB_MAX_SRV_NAME_LEN);
                name[AM_DB_MAX_SRV_NAME_LEN] = 0;
                LOGD("found name [%s]", name);

                /*3bytes language code, using xxx to simulate*/
                COPY_NAME("xxx", 3);
                /*following by name text*/
                tmp_len = strlen(name);
                COPY_NAME(name, tmp_len);
            }
            /*业务类型*/
            srv_info->srv_type = psd->i_service_type;
            /*service type 0x16 and 0x19 is user defined, as digital television service*/
            /*service type 0xc0 is type of partial reception service in ISDBT*/
            if ((srv_info->srv_type == 0x16) || (srv_info->srv_type == 0x19) || (srv_info->srv_type == 0xc0)) {
                srv_info->srv_type = 0x1;
            }
            break;
        }
        AM_SI_LIST_END()

        /* store multilingual service name */
        AM_SI_LIST_BEGIN(srv->p_first_descriptor, descr)
        if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_MULTI_SERVICE_NAME) {
            int i;
            dvbpsi_multi_service_name_dr_t *pmsnd = (dvbpsi_multi_service_name_dr_t *)descr->p_decoded;

            for (i = 0; i < pmsnd->i_name_count; i++) {
                name[0] = 0;
                AM_SI_ConvertDVBTextCode((char *)pmsnd->p_service_name[i].i_service_name,
                                         pmsnd->p_service_name[i].i_service_name_length,
                                         name, AM_DB_MAX_SRV_NAME_LEN);
                name[AM_DB_MAX_SRV_NAME_LEN] = 0;
                LOGD("found name [%s]", name);

                if (curr_name_len > 0) {
                    /*extra split mark*/
                    COPY_NAME(&split, 1);
                }
                /*3bytes language code*/
                COPY_NAME(pmsnd->p_service_name[i].i_iso_639_code, 3);
                /*following by name text*/
                tmp_len = strlen(name);
                COPY_NAME(name, tmp_len);
            }
        }
        AM_SI_LIST_END()

        /* set the ending null byte */
        if (curr_name_len >= name_size)
            srv_info->name[name_size - 1] = 0;
        else
            srv_info->name[curr_name_len] = 0;

        break;
    }
    AM_SI_LIST_END()
    AM_SI_LIST_END()
}

void CTvScanner::scan_update_service_info(AM_SCAN_Result_t *result, SCAN_ServiceInfo_t *srv_info)
{
#define str(i) (char*)(strings + i)

    static char strings[14][256];

    if (srv_info->src != FE_ANALOG) {
        int standard = result->start_para->dtv_para.standard;
        int mode = result->start_para->dtv_para.mode;

        /* Transform service types for different dtv standards */
        if (standard != AM_SCAN_DTV_STD_ATSC) {
            if (srv_info->srv_type == 0x1)
                srv_info->srv_type = AM_SCAN_SRV_DTV;
            else if (srv_info->srv_type == 0x2)
                srv_info->srv_type = AM_SCAN_SRV_DRADIO;
        } else {
            if (srv_info->srv_type == 0x2)
                srv_info->srv_type = AM_SCAN_SRV_DTV;
            else if (srv_info->srv_type == 0x3)
                srv_info->srv_type = AM_SCAN_SRV_DRADIO;
        }

        /* if video valid, set this program to tv type,
         * if audio valid, but video not found, set it to radio type,
         * if both invalid, but service_type found in SDT/VCT, set to unknown service,
         * this mechanism is OPTIONAL
         */
        if (srv_info->vid < 0x1fff) {
            srv_info->srv_type = AM_SCAN_SRV_DTV;
        } else if (srv_info->aud_info.audio_count > 0) {
            srv_info->srv_type = AM_SCAN_SRV_DRADIO;
        } else if (srv_info->srv_type == AM_SCAN_SRV_DTV ||
                   srv_info->srv_type == AM_SCAN_SRV_DRADIO) {
            srv_info->srv_type = AM_SCAN_SRV_UNKNOWN;
        }
        /* Skip program for FTA mode */
        if (srv_info->scrambled_flag && (mode & AM_SCAN_DTVMODE_FTA)) {
            LOGD( "Skip program '%s' for FTA mode", srv_info->name);
            return;
        }

        /* Skip program for service_type mode */
        if (srv_info->srv_type == AM_SCAN_SRV_DTV && (mode & AM_SCAN_DTVMODE_NOTV)) {
            LOGD( "Skip program '%s' for NO-TV mode", srv_info->name);
            return;
        }
        if (srv_info->srv_type == AM_SCAN_SRV_DRADIO && (mode & AM_SCAN_DTVMODE_NORADIO)) {
            LOGD( "Skip program '%s' for NO-RADIO mode", srv_info->name);
            return;
        }

        /* Set default name to tv/radio program if no name specified */
        if (!strcmp(srv_info->name, "") &&
                (srv_info->srv_type == AM_SCAN_SRV_DTV ||
                 srv_info->srv_type == AM_SCAN_SRV_DRADIO)) {
            strcpy(srv_info->name, "xxxNo Name");
        }
    }
}

void CTvScanner::scan_store_dvb_ts_evt_service(SCAN_ServiceInfo_t *srv)
{
    LOGD("scan_store_dvb_ts_evt_service freq:%d, sid:%d", m_s_Scanner->mCurEv.mFrequency, srv->srv_id);
    m_s_Scanner->mCurEv.clear();
    m_s_Scanner->mCurEv.mServiceId = srv->srv_id;
    strncpy(m_s_Scanner->mCurEv.mProgramName, srv->name, 1024);
    m_s_Scanner->mCurEv.mprogramType = srv->srv_type;
    m_s_Scanner->mCurEv.mVid = srv->vid;
    m_s_Scanner->mCurEv.mVfmt = srv->vfmt;
    m_s_Scanner->mCurEv.mAcnt = srv->aud_info.audio_count;
    for (int i = 0; i < srv->aud_info.audio_count; i++) {
        m_s_Scanner->mCurEv.mAid[i] = srv->aud_info.audios[i].pid;
        m_s_Scanner->mCurEv.mAfmt[i] = srv->aud_info.audios[i].fmt;
        strncpy(m_s_Scanner->mCurEv.mAlang[i], srv->aud_info.audios[i].lang, 10);
        m_s_Scanner->mCurEv.mAtype[i] = srv->aud_info.audios[i].audio_type;
    }
    m_s_Scanner->mCurEv.mPcr = srv->pcr_pid;
    m_s_Scanner->mCurEv.mScnt = srv->sub_info.subtitle_count;
    for (int i = 0; i < srv->sub_info.subtitle_count; i++) {
        m_s_Scanner->mCurEv.mStype[i] = TYPE_DVB_SUBTITLE;
        m_s_Scanner->mCurEv.mSid[i] = srv->sub_info.subtitles[i].pid;
        m_s_Scanner->mCurEv.mSstype[i] = srv->sub_info.subtitles[i].type;
        m_s_Scanner->mCurEv.mSid1[i] = srv->sub_info.subtitles[i].comp_page_id;
        m_s_Scanner->mCurEv.mSid2[i] = srv->sub_info.subtitles[i].anci_page_id;
        strncpy(m_s_Scanner->mCurEv.mSlang[i], srv->sub_info.subtitles[i].lang, 10);
    }
    int scnt = m_s_Scanner->mCurEv.mScnt;
    for (int i = 0; i < srv->ttx_info.teletext_count; i++) {
        if (srv->ttx_info.teletexts[i].type == 0x2 ||
                srv->ttx_info.teletexts[i].type == 0x5) {
            if (scnt >= (sizeof(m_s_Scanner->mCurEv.mStype) / sizeof(int)))
                break;
            m_s_Scanner->mCurEv.mStype[scnt] = TYPE_DTV_TELETEXT;
            m_s_Scanner->mCurEv.mSid[scnt] = srv->ttx_info.teletexts[i].pid;
            m_s_Scanner->mCurEv.mSstype[scnt] = srv->ttx_info.teletexts[i].type;
            m_s_Scanner->mCurEv.mSid1[scnt] = srv->ttx_info.teletexts[i].magazine_no;
            m_s_Scanner->mCurEv.mSid2[scnt] = srv->ttx_info.teletexts[i].page_no;
            strncpy(m_s_Scanner->mCurEv.mSlang[scnt], srv->ttx_info.teletexts[i].lang, 10);
            scnt++;
        }
    }
    m_s_Scanner->mCurEv.mScnt = scnt;

    m_s_Scanner->mCurEv.mMSG[0] = '\0';

    m_s_Scanner->mCurEv.mType = ScannerEvent::EVENT_DTV_PROG_DATA;
    m_s_Scanner->sendEvent(m_s_Scanner->mCurEv);
}

void CTvScanner::scan_store_dvb_ts(AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts)
{
    dvbpsi_pmt_t *pmt;
    dvbpsi_pmt_es_t *es;
    dvbpsi_descriptor_t *descr;
    int src = result->start_para->dtv_para.source;
    int mode = result->start_para->dtv_para.mode;
    int net_dbid = -1, dbid = -1, orig_net_id = -1, satpara_dbid = -1;
    char selbuf[256];
    char insbuf[400];
    AM_Bool_t store = AM_TRUE;
    dvbpsi_pat_t *valid_pat = NULL;
    uint8_t plp_id;
    SCAN_ServiceInfo_t srv_info;

    valid_pat = get_valid_pats(ts);
    if (valid_pat == NULL) {
        LOGD("No PAT found in ts, will not store to dbase");
        return;
    }

    LOGD("@@ TS: src %d @@", src);

    scan_process_ts_info(result, ts, &m_s_Scanner->mCurEv);

    if (ts->digital.pmts || (IS_DVBT2_TS(ts->digital.fend_para) && ts->digital.dvbt2_data_plp_num > 0)) {
        int loop_count, lc;
        dvbpsi_sdt_t *sdt_list;
        dvbpsi_pmt_t *pmt_list;
        dvbpsi_pat_t *pat_list;

        /* For DVB-T2, search for each PLP, else search in current TS*/
        loop_count = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plp_num : 1;
        LOGD("plp num %d", loop_count);

        for (lc = 0; lc < loop_count; lc++) {
            pat_list = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plps[lc].pats : ts->digital.pats;
            pmt_list = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plps[lc].pmts : ts->digital.pmts;
            sdt_list =  IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plps[lc].sdts : ts->digital.sdts;
            plp_id = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plps[lc].id : -1;
            LOGD("plp_id %d", plp_id);

            AM_SI_LIST_BEGIN(pmt_list, pmt) {
                scan_init_service_info(&srv_info);
                srv_info.satpara_dbid = satpara_dbid;
                srv_info.srv_id = pmt->i_program_number;
                srv_info.src = src;
                srv_info.pmt_pid = get_pmt_pid(pat_list, pmt->i_program_number);
                srv_info.pcr_pid = pmt->i_pcr_pid;
                srv_info.plp_id  = plp_id;

                /* looking for CA descr */
                if (! srv_info.scrambled_flag) {
                    scan_extract_ca_scrambled_flag(pmt->p_first_descriptor, &srv_info.scrambled_flag);
                }

                AM_SI_LIST_BEGIN(pmt->p_first_es, es) {
                    AM_SI_ExtractAVFromES(es, &srv_info.vid, &srv_info.vfmt, &srv_info.aud_info);

                    if (store) {
                        AM_SI_ExtractDVBSubtitleFromES(es, &srv_info.sub_info);
                        AM_SI_ExtractDVBTeletextFromES(es, &srv_info.ttx_info);
                    }

                    if (! srv_info.scrambled_flag)
                        scan_extract_ca_scrambled_flag(es->p_first_descriptor, &srv_info.scrambled_flag);
                }
                AM_SI_LIST_END()

                scan_extract_srv_info_from_sdt(result, sdt_list, &srv_info);

                /*Store this service*/
                scan_update_service_info(result, &srv_info);

                scan_store_dvb_ts_evt_service(&srv_info);

            }
            AM_SI_LIST_END()

            /* All programs in PMTs added, now trying the programs in SDT but NOT in PMT */
            dvbpsi_sdt_service_t *srv;
            dvbpsi_sdt_t *sdt;

            AM_SI_LIST_BEGIN(ts->digital.sdts, sdt) {
                AM_SI_LIST_BEGIN(sdt->p_first_service, srv) {
                    AM_Bool_t found_in_pmt = AM_FALSE;

                    /* Is already added in PMT? */
                    AM_SI_LIST_BEGIN(ts->digital.pmts, pmt)
                    if (srv->i_service_id == pmt->i_program_number) {
                        found_in_pmt = AM_TRUE;
                        break;
                    }
                    AM_SI_LIST_END()

                    if (found_in_pmt)
                        continue;

                    scan_init_service_info(&srv_info);
                    srv_info.satpara_dbid = satpara_dbid;
                    srv_info.srv_id = srv->i_service_id;
                    srv_info.src = src;

                    scan_extract_srv_info_from_sdt(result, sdt_list, &srv_info);

                    scan_update_service_info(result, &srv_info);

                    /*as no pmt for this srv, set type to data for invisible*/
                    srv_info.srv_type = 0;

                    scan_store_dvb_ts_evt_service(&srv_info);

                }
                AM_SI_LIST_END()
            }
            AM_SI_LIST_END()

        }
    }
}

void CTvScanner::dtv_scan_store(AM_SCAN_Result_t *result)
{
    AM_SCAN_TS_t *ts;

    LOGD("Storing tses ...");

    AM_SI_LIST_BEGIN(result->tses, ts) {
        scan_store_dvb_ts(result, ts);
    }
    AM_SI_LIST_END()
}

int CTvScanner::manualDtmbScan(int beginFreq, int endFreq, int modulation)
{
    stopScan();

    if (!mbScanStart) { //not start or stop
        AM_SCAN_CreatePara_t para;
        AM_DMX_OpenPara_t dmx_para;
        AM_SCAN_Handle_t handle = 0;
        int i;
        // Create the scan
        memset(&para, 0, sizeof(para));

        para.fend_dev_id = 0;//default
        para.mode = AM_SCAN_MODE_DTV_ATV;//DTV
        para.atv_para.mode = AM_SCAN_ATVMODE_NONE;
        para.dtv_para.mode = AM_SCAN_DTVMODE_MANUAL;
        para.dtv_para.source = FE_DTMB;//fe_type
        para.dtv_para.dmx_dev_id = 0;//default 0

        Vector<sp<CTvChannel> > vcp;
        CTvRegion::getChannelListByNameAndFreqRange("CHINA,Default DTMB ALL", beginFreq, endFreq, vcp);
        int size = vcp.size();

        //@author:hao.fu
        if (size == 0) {
            return -1;
        }

        if (!(para.dtv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t *>(calloc(size, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
            return -1;

        for (i = 0; i < size; i++) {
            para.dtv_para.fe_paras[i].m_type = 5;//MODE_DTMB
            para.dtv_para.fe_paras[i].dtmb.para.frequency = vcp[i]->getFrequency();
            para.dtv_para.fe_paras[i].dtmb.para.inversion = INVERSION_OFF;
            para.dtv_para.fe_paras[i].dtmb.para.u.ofdm.bandwidth = (fe_bandwidth_t)(vcp[i]->getBandwidth());
            if (modulation == -1)
                para.dtv_para.fe_paras[i].cable.para.u.qam.modulation = (fe_modulation_t)(vcp[i]->getModulation());
            else
                para.dtv_para.fe_paras[i].cable.para.u.qam.modulation = (fe_modulation_t)modulation;
        }

        para.dtv_para.fe_cnt = size;
        para.dtv_para.resort_all = AM_FALSE;
        para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
        para.store_cb = dtv_scan_store;


        memset(&dmx_para, 0, sizeof(dmx_para));
        AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

        if ((para.dtv_para.mode & 0x07) != AM_SCAN_DTVMODE_NONE) {
            AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
            tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
        }

        // Start Scan
        if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
            LOGD("SCAN CREATE fail");
            handle = NULL;
        } else {
            mScanHandle = handle;
            AM_SCAN_SetUserData(handle, (void *)this);
            AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
            AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
            if (AM_SCAN_Start(handle) != AM_SUCCESS) {
                AM_SCAN_Destroy(handle, AM_FALSE);
                AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
                AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
                handle = NULL;
            }
        }
        if (para.atv_para.fe_paras != NULL)
            free(para.atv_para.fe_paras);
        if (para.dtv_para.fe_paras != NULL)
            free(para.dtv_para.fe_paras);

        if (handle == NULL) {
            return -1;
        }
        mbScanStart = true;//start call ok
    } else {
        LOGD("scanning and not stop");
    }
    return 0;
}


/*int CTvScanner::autoTvScan()
{
    stopScan();

    if(!mbScanStart) { //not start or stop
        AM_SCAN_CreatePara_t para;
        AM_DMX_OpenPara_t dmx_para;
        int handle = 0,i;
        // Create the scan
        memset(&para, 0, sizeof(para));

        //strcpy(para.default_text_lang,"eng");//config
        //strcpy(para.text_langs, "eng zho chi");//config
        para.fend_dev_id = 0;//default
        para.mode = AM_SCAN_MODE_DTV_ATV;//DTV
        para.atv_para.mode = AM_SCAN_ATVMODE_NONE;
        para.dtv_para.mode = AM_SCAN_DTVMODE_ALLBAND;//DTV_MODE_ALLBAND
        //para.dtv_para.mode |= (*env)->GetIntField(env, para, doptions);//忽略
        para.dtv_para.source = 1;//QAM
        para.dtv_para.dmx_dev_id = 0;//default 0
        Vector<sp<CTvChannel> > vcp;
        //CTvDatabase::getChannelParaList("/data/tv_default.xml", vcp);//channel list from xml or db
        CTvRegion::getChannelListByName("China,DVB-C allband", vcp);

        int size = vcp.size();
        LOGD("channel list size = %d", size);
        if(!(para.dtv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t*>(calloc(size, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
            return -1;

        //memset(pfp, 0, size * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));

        for (i = 0; i < size; i++) {
            para.dtv_para.fe_paras[i].m_type = FE_QAM;
            para.dtv_para.fe_paras[i].cable.para.frequency = vcp[i]->getFrequency();
            para.dtv_para.fe_paras[i].cable.para.inversion = INVERSION_OFF;
            para.dtv_para.fe_paras[i].cable.para.u.qam.symbol_rate = vcp[i]->getSymbolRate();
            para.dtv_para.fe_paras[i].cable.para.u.qam.modulation = (fe_modulation_t)(vcp[i]->getModulation());
        }
        //allband
        para.dtv_para.fe_cnt = size;

        para.dtv_para.resort_all = AM_FALSE;
        para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
        para.store_cb = NULL;
        memset(&dmx_para, 0, sizeof(dmx_para));
        AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

        if ((para.dtv_para.mode&0x07) != AM_SCAN_DTVMODE_NONE) {
            AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
            tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
        }

        //    prog->dmx_id = para.dtv_para.dmx_dev_id;
        //    prog->fend_id = para.fend_dev_id;
        //    prog->mode = para.dtv_para.mode;
        // Start Scan
        if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
            LOGD("SCAN CREATE fail");
            handle = 0;
        } else {
            LOGD("SCAN CREATE handle = %d", handle);
            mScanHandle = handle;
            AM_SCAN_SetUserData(handle, (void*)this);
            //注册搜索事件
            AM_EVT_Subscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
            //注册信号质量通知事件
            //AM_EVT_Subscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
            if (AM_SCAN_Start(handle) != AM_SUCCESS) {
                AM_SCAN_Destroy(handle, AM_FALSE);
                AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
                //    AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
                handle = 0;
            }
        }
        if (para.atv_para.fe_paras != NULL)
            free(para.atv_para.fe_paras);
        if (para.dtv_para.fe_paras != NULL)
            free(para.dtv_para.fe_paras);

        if (handle == 0) {
            return -1;
        }
        mbScanStart = true;//start call ok
    } else {
        LOGD("scanning and not stop");
    }
    return 0;
}*/
//only test for dtv allbland auto
/*int CTvScanner::startScan()
{
    stopScan();

    if(!mbScanStart) { //not start or stop
        AM_SCAN_CreatePara_t para;
        AM_DMX_OpenPara_t dmx_para;
        int handle = 0,i;
        // Create the scan
        memset(&para, 0, sizeof(para));

        //strcpy(para.default_text_lang,"eng");//config
        //strcpy(para.text_langs, "eng zho chi");//config
        para.fend_dev_id = 0;//default
        para.mode = AM_SCAN_MODE_DTV_ATV;//DTV
        para.atv_para.mode = AM_SCAN_ATVMODE_NONE;
        para.dtv_para.mode = 3;//DTV_MODE_ALLBAND
        //para.dtv_para.mode |= (*env)->GetIntField(env, para, doptions);//忽略
        para.dtv_para.source = 1;//QAM
        para.dtv_para.dmx_dev_id = 0;//default 0
        Vector<sp<CTvChannel> > vcp;
        //CTvDatabase::getChannelParaList("/data/tv_default.xml", vcp);//channel list from xml or db
        CTvRegion::getChannelListByName("China,DVB-C allband", vcp);

        int size = vcp.size();
        LOGD("channel list size = %d", size);
        if(!(para.dtv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t*>(calloc(size, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
            return -1;

        //memset(pfp, 0, size * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));

        for (i = 0; i < size; i++) {
            para.dtv_para.fe_paras[i].m_type = FE_QAM;
            para.dtv_para.fe_paras[i].cable.para.frequency = vcp[i]->getFrequency();
            para.dtv_para.fe_paras[i].cable.para.inversion = INVERSION_OFF;
            para.dtv_para.fe_paras[i].cable.para.u.qam.symbol_rate = vcp[i]->getSymbolRate();
            para.dtv_para.fe_paras[i].cable.para.u.qam.modulation = (fe_modulation_t)(vcp[i]->getModulation());
        }
        //allband
        para.dtv_para.fe_cnt = size;

        para.dtv_para.resort_all = AM_FALSE;
        para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
        para.store_cb = NULL;

        memset(&dmx_para, 0, sizeof(dmx_para));
        AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

        if ((para.dtv_para.mode&0x07) != AM_SCAN_DTVMODE_NONE) {
            AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
            tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
        }

        //    prog->dmx_id = para.dtv_para.dmx_dev_id;
        //    prog->fend_id = para.fend_dev_id;
        //    prog->mode = para.dtv_para.mode;
        // Start Scan
        if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
            LOGD("SCAN CREATE fail");
            handle = 0;
        } else {
            mScanHandle = handle;
            AM_SCAN_SetUserData(handle, (void*)this);
            //注册搜索事件
            AM_EVT_Subscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
            //注册信号质量通知事件
            //AM_EVT_Subscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
            if (AM_SCAN_Start(handle) != AM_SUCCESS) {
                AM_SCAN_Destroy(handle, AM_FALSE);
                AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
                //    AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
                handle = 0;
            }
        }
        if (para.atv_para.fe_paras != NULL)
            free(para.atv_para.fe_paras);
        if (para.dtv_para.fe_paras != NULL)
            free(para.dtv_para.fe_paras);

        if (handle == 0) {
            return -1;
        }
        mbScanStart = true;//start call ok
    } else {
        LOGD("scanning and not stop");
    }
    return 0;
}*/


//only test for dtv allbland auto
int CTvScanner::autoDtmbScan()
{
    stopScan();

    if (!mbScanStart) { //not start or stop
        AM_SCAN_CreatePara_t para;
        AM_DMX_OpenPara_t dmx_para;
        AM_SCAN_Handle_t handle = 0;
        // Create the scan
        memset(&para, 0, sizeof(para));

        //strcpy(para.default_text_lang,"eng");//config
        //strcpy(para.text_langs, "local eng zho chi chs first");//config
        para.fend_dev_id = 0;//default
        para.mode = AM_SCAN_MODE_DTV_ATV;//DTV
        para.atv_para.mode = AM_SCAN_ATVMODE_NONE;
        para.dtv_para.mode = AM_SCAN_DTVMODE_ALLBAND;//DTV_MODE_ALLBAND
        //para.dtv_para.mode |= (*env)->GetIntField(env, para, doptions);//忽略
        para.dtv_para.source = FE_DTMB;//fe_type
        para.dtv_para.dmx_dev_id = 0;//default 0
        Vector<sp<CTvChannel> > vcp;
        //CTvDatabase::getChannelParaList("/data/tv_default.xml", vcp);//channel list from xml or db
        CTvRegion::getChannelListByName("CHINA,Default DTMB ALL", vcp);

        int size = vcp.size();
        LOGD("channel list size = %d", size);
        //showboz
        if (size == 0) {
            CTvDatabase::GetTvDb()->importXmlToDB("/etc/tv_default.xml");
            CTvRegion::getChannelListByName("CHINA,Default DTMB ALL", vcp);
            size = vcp.size();
        }

        if (!(para.dtv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t *>(calloc(size, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
            return -1;

        //memset(pfp, 0, size * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));

        for (int i = 0; i < size; i++) {
            para.dtv_para.fe_paras[i].m_type = 5;//MODE_DTMB
            para.dtv_para.fe_paras[i].dtmb.para.frequency = vcp[i]->getFrequency();
            para.dtv_para.fe_paras[i].dtmb.para.inversion = INVERSION_OFF;
            para.dtv_para.fe_paras[i].dtmb.para.u.ofdm.bandwidth = (fe_bandwidth_t)(vcp[i]->getBandwidth());
        }
        //allband
        para.dtv_para.fe_cnt = size;

        para.dtv_para.resort_all = AM_FALSE;
        para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
        para.store_cb = dtv_scan_store;

        memset(&dmx_para, 0, sizeof(dmx_para));
        AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

        if ((para.dtv_para.mode & 0x07) != AM_SCAN_DTVMODE_NONE) {
            AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
            tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
        }

        //    prog->dmx_id = para.dtv_para.dmx_dev_id;
        //    prog->fend_id = para.fend_dev_id;
        //    prog->mode = para.dtv_para.mode;
        // Start Scan
        if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
            LOGD("SCAN CREATE fail");
            handle = NULL;
        } else {
            mScanHandle = handle;
            AM_SCAN_SetUserData(handle, (void *)this);
            //注册搜索事件
            AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
            //注册信号质量通知事件
            AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
            if (AM_SCAN_Start(handle) != AM_SUCCESS) {
                AM_SCAN_Destroy(handle, AM_FALSE);
                AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
                AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
                handle = NULL;
            }
        }
        if (para.atv_para.fe_paras != NULL)
            free(para.atv_para.fe_paras);
        if (para.dtv_para.fe_paras != NULL)
            free(para.dtv_para.fe_paras);

        if (handle == NULL) {
            return -1;
        }
        mbScanStart = true;//start call ok
    } else {
        LOGD("scanning and not stop");
    }
    return 0;
}

/*int CTvScanner::autoAtscScan(int attennaType,int std)
{
    stopScan();

    if(!mbScanStart) { //not start or stop
        AM_SCAN_CreatePara_t para;
        AM_DMX_OpenPara_t dmx_para;
        int handle = 0,i;
        // Create the scan
        memset(&para, 0, sizeof(para));

        //strcpy(para.default_text_lang,"eng");//config
        //strcpy(para.text_langs, "local eng zho chi chs first");//config
        para.fend_dev_id = 0;//default
        para.mode = AM_SCAN_MODE_ADTV;//DTV
        para.atv_para.mode = AM_SCAN_ATVMODE_NONE;


        para.atv_para.default_std= std;


        para.dtv_para.mode = AM_SCAN_DTVMODE_ALLBAND;//DTV_MODE_ALLBAND
        //para.dtv_para.mode |= (*env)->GetIntField(env, para, doptions);//忽略
        para.dtv_para.source = FE_ATSC;//fe_type
        para.dtv_para.dmx_dev_id = 0;//default 0
        para.dtv_para.resort_all = AM_TRUE;
        para.dtv_para.clear_source = AM_TRUE;
        para.dtv_para.mix_tv_radio = AM_FALSE;
        para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
        para.store_cb = NULL;
        //para.dtv_para.attenna = attennaType;//see dvb atsc-cnc

        para.dtv_para.standard = AM_SCAN_DTV_STD_ATSC;

        mSource = para.dtv_para.source;

        Vector<sp<CTvChannel> > vcp;
        //CTvDatabase::getChannelParaList("/data/tv_default.xml", vcp);//channel list from xml or db
        getAtscChannelPara(attennaType,vcp);
        LOGD("ANDESEN test attenna type is %d",attennaType);

        int size = vcp.size();
        LOGD("channel list size = %d", size);
        //showboz
        if(size == 0) {
            CTvDatabase::GetTvDb()->importXmlToDB("/etc/tv_default.xml");
            getAtscChannelPara(attennaType,vcp);
            size = vcp.size();
        }

        if(!(para.dtv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t*>(calloc(size, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
            return -1;

        //memset(pfp, 0, size * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));

        for (i = 0; i < size; i++) {
            para.dtv_para.fe_paras[i].m_type = 3;//MODE_ATSC
            para.dtv_para.fe_paras[i].m_logicalChannelNum = vcp[i]->getLogicalChannelNum();
            para.dtv_para.fe_paras[i].atsc.para.frequency = vcp[i]->getFrequency();
            para.dtv_para.fe_paras[i].atsc.para.u.vsb.modulation = (fe_modulation_t)(vcp[i]->getModulation());
        }
        //allband
        para.dtv_para.fe_cnt = size;

        memset(&dmx_para, 0, sizeof(dmx_para));
        AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

        if ((para.dtv_para.mode&0x07) != AM_SCAN_DTVMODE_NONE) {
            AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
            tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
        }

        //    prog->dmx_id = para.dtv_para.dmx_dev_id;
        //    prog->fend_id = para.fend_dev_id;
        //    prog->mode = para.dtv_para.mode;
        // Start Scan
        if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
            LOGD("SCAN CREATE fail");
            handle = 0;
        } else {
            mScanHandle = handle;
            AM_SCAN_SetUserData(handle, (void*)this);
            //注册搜索事件
            AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
            AM_EVT_Subscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
            //注册信号质量通知事件
            AM_EVT_Subscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
            if (AM_SCAN_Start(handle) != AM_SUCCESS) {
                AM_SCAN_Destroy(handle, AM_FALSE);
                AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
                AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
                handle = 0;
            }
        }
        if (para.atv_para.fe_paras != NULL)
            free(para.atv_para.fe_paras);
        if (para.dtv_para.fe_paras != NULL)
            free(para.dtv_para.fe_paras);

        if (handle == 0) {
            return -1;
        }
        mbScanStart = true;//start call ok
    } else {
        LOGD("scanning and not stop");
    }
    return 0;
}*/


int CTvScanner::stopScan()
{
    LOGD("StopScan mScanHandle=%d", mScanHandle);
    //requestExit();
    if (mbScanStart) { //if start ok and not stop
        int ret = AM_SCAN_Destroy(mScanHandle, AM_TRUE);
        AM_EVT_Unsubscribe((long)mScanHandle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
        AM_EVT_Unsubscribe((long)mScanHandle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
        AM_SEC_Cache_Reset(0);
        //stop loop
        LOGD("StopScan , ret=%d", ret);
        mbScanStart = false;//stop ok
    } else {
        LOGD("not start scan or scan stoped");
    }
    return 0;
}

void CTvScanner::tv_scan_evt_callback(long dev_no, int event_type, void *param, void *data)
{
    CTvScanner *pT = NULL;
    long long tmpFreq = 0;

    LOGD("evt evt:%d", event_type);
    AM_SCAN_GetUserData((AM_SCAN_Handle_t)dev_no, (void **)&pT);
    if (pT == NULL) {
        return;
    }
    pT->mCurEv.clear();
    memset(pT->mCurEv.mProgramName, '\0', sizeof(pT->mCurEv.mProgramName));
    memset(pT->mCurEv.mMSG, '\0', sizeof(pT->mCurEv.mMSG));
    if (event_type == AM_SCAN_EVT_PROGRESS) {
        AM_SCAN_Progress_t *evt = (AM_SCAN_Progress_t *)param;
        LOGD("progress evt:%d", evt->evt);
        switch (evt->evt) {
        case AM_SCAN_PROGRESS_SCAN_BEGIN:
            pT->mCurEv.mPercent = 0;
            break;
        case AM_SCAN_PROGRESS_NIT_BEGIN:

            break;
        case AM_SCAN_PROGRESS_NIT_END:

            break;
        case AM_SCAN_PROGRESS_TS_BEGIN: {
            AM_SCAN_TSProgress_t *tp = (AM_SCAN_TSProgress_t *)evt->data;
            if (tp == NULL)
                break;
            pT->mCurEv.mChannelNumber = tp->index;
            pT->mCurEv.mMode = tp->fend_para.m_type;
            pT->mCurEv.mFrequency = ((struct dvb_frontend_parameters *)(&tp->fend_para))->frequency;
            pT->mCurEv.mSymbolRate = tp->fend_para.cable.para.u.qam.symbol_rate;
            pT->mCurEv.mModulation = tp->fend_para.cable.para.u.qam.modulation;
            pT->mCurEv.mBandwidth = tp->fend_para.terrestrial.para.u.ofdm.bandwidth;
            pT->mCurEv.mOfdm_mode = tp->fend_para.terrestrial.para.u.ofdm.ofdm_mode;
            pT->mCurEv.mAudio = tp->fend_para.analog.para.u.analog.audmode;
            pT->mCurEv.mStandard = tp->fend_para.analog.para.u.analog.std;
            pT->mCurEv.mSat_polarisation = tp->fend_para.sat.polarisation;
            if (pT->mCurEv.mMode == FE_ANALOG) {
                if (pT->mSource == FE_ATSC) {
                    pT->mCurEv.mPercent = (tp->index * 100) / tp->total;
                } else {
                    pT->mCurEv.mPercent = 0;
                }
            } else {
                pT->mCurEv.mPercent = (tp->index * 100) / tp->total;
            }

            if (pT->mCurEv.mTotalChannelCount == 0)
                pT->mCurEv.mTotalChannelCount = tp->total;
            if (pT->mCurEv.mPercent >= 100)
                pT->mCurEv.mPercent = 99;

            pT->mCurEv.mLockedStatus = 0;
            pT->mCurEv.mStrength = 0;
            pT->mCurEv.mSnr = 0;
            pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

            pT->sendEvent(pT->mCurEv);
        }
        break;
        case AM_SCAN_PROGRESS_TS_END: {
            /*pT->mCurEv.mLockedStatus = 0;
            pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
            pT->sendEvent(pT->mCurEv);*/
        }
        break;

        case AM_SCAN_PROGRESS_PAT_DONE: /*{
                if(pT->mCurEv.mTotalChannelCount == 1) {
                    pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
                    pT->sendEvent(pT->mCurEv);
                }
            }*/
            break;
        case AM_SCAN_PROGRESS_SDT_DONE: /*{
                dvbpsi_sdt_t *sdts = (dvbpsi_sdt_t *)evt->data;
                dvbpsi_sdt_t *sdt;

                if (pT->mCurEv.mTotalChannelCount == 1) {
                    pT->mCurEv.mPercent += 25;
                    if (pT->mCurEv.mPercent >= 100)
                        pT->mCurEv.mPercent = 99;
                    pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

                    pT->sendEvent(pT->mCurEv);
                }
            }*/
            break;
        case AM_SCAN_PROGRESS_CAT_DONE: /*{
                dvbpsi_cat_t *cat = (dvbpsi_cat_t *)evt->data;
                if (pT->mCurEv.mTotalChannelCount == 1) {
                    pT->mCurEv.mPercent += 25;
                    if (pT->mCurEv.mPercent >= 100)
                        pT->mCurEv.mPercent = 99;

                    pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

                    pT->sendEvent(pT->mCurEv);
                }
            }*/
            break;
        case AM_SCAN_PROGRESS_PMT_DONE: /*{
                dvbpsi_pmt_t *pmt = (dvbpsi_pmt_t *)evt->data;
                if (pT->mCurEv.mTotalChannelCount == 1) {
                    pT->mCurEv.mPercent += 25;
                    if (pT->mCurEv.mPercent >= 100)
                        pT->mCurEv.mPercent = 99;

                    pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
                    pT->sendEvent(pT->mCurEv);
                }
            }*/
            break;
        case AM_SCAN_PROGRESS_MGT_DONE: {
            mgt_section_info_t *mgt = (mgt_section_info_t *)evt->data;

            if (pT->mCurEv.mTotalChannelCount == 1) {
                pT->mCurEv.mPercent += 10;
                if (pT->mCurEv.mPercent >= 100)
                    pT->mCurEv.mPercent = 99;

                pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

                pT->sendEvent(pT->mCurEv);
            }
        }
        break;
        case AM_SCAN_PROGRESS_VCT_DONE: {
            /*ATSC TVCT*/
            if (pT->mCurEv.mTotalChannelCount == 1) {
                pT->mCurEv.mPercent += 30;
                if (pT->mCurEv.mPercent >= 100)
                    pT->mCurEv.mPercent = 99;
                pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

                pT->sendEvent(pT->mCurEv);
            }
        }
        break;
        case AM_SCAN_PROGRESS_NEW_PROGRAM: {
            /* Notify the new searched programs */
            AM_SCAN_ProgramProgress_t *pp = (AM_SCAN_ProgramProgress_t *)evt->data;
            if (pp != NULL) {
                pT->mCurEv.mprogramType = pp->service_type;
                snprintf(pT->mCurEv.mProgramName, sizeof(pT->mCurEv.mProgramName), "%s", pp->name);

                pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

                pT->sendEvent(pT->mCurEv);
            }
        }
        break;
        case AM_SCAN_PROGRESS_BLIND_SCAN: {
            AM_SCAN_DTVBlindScanProgress_t *bs_prog = (AM_SCAN_DTVBlindScanProgress_t *)evt->data;

            if (bs_prog) {
                pT->mCurEv.mPercent = bs_prog->progress;

                snprintf(pT->mCurEv.mMSG, sizeof(pT->mCurEv.mMSG), "%s/%s %dMHz",
                         bs_prog->polar == AM_FEND_POLARISATION_H ? "H" : "V",
                         bs_prog->lo == AM_FEND_LOCALOSCILLATORFREQ_L ? "L-LOF" : "H-LOF",
                         bs_prog->freq / 1000);

                pT->mCurEv.mType = ScannerEvent::EVENT_BLINDSCAN_PROGRESS;

                pT->sendEvent(pT->mCurEv);

                if (bs_prog->new_tp_cnt > 0) {
                    int i = 0;
                    for (i = 0; i < bs_prog->new_tp_cnt; i++) {
                        LOGD("====New tp: %dkS/s %d====", bs_prog->new_tps[i].frequency,
                             bs_prog->new_tps[i].u.qpsk.symbol_rate);

                        pT->mCurEv.mFrequency = bs_prog->new_tps[i].frequency;
                        pT->mCurEv.mSymbolRate = bs_prog->new_tps[i].u.qpsk.symbol_rate;
                        pT->mCurEv.mSat_polarisation = bs_prog->polar;

                        pT->mCurEv.mType = ScannerEvent::EVENT_BLINDSCAN_NEWCHANNEL;

                        pT->sendEvent(pT->mCurEv);
                    }
                }
                if (bs_prog->progress >= 100) {

                    pT->mCurEv.mType = ScannerEvent::EVENT_BLINDSCAN_END;

                    pT->sendEvent(pT->mCurEv);

                    pT->mCurEv.mPercent = 0;
                }
            }
        }
        break;
        case AM_SCAN_PROGRESS_STORE_BEGIN: {
            pT->mCurEv.mType = ScannerEvent::EVENT_STORE_BEGIN;
            pT->mCurEv.mLockedStatus = 0;
            pT->sendEvent(pT->mCurEv);
        }
        break;
        case AM_SCAN_PROGRESS_STORE_END: {
            pT->mCurEv.mLockedStatus = 0;
            pT->mCurEv.mType = ScannerEvent::EVENT_STORE_END;
            pT->sendEvent(pT->mCurEv);
        }
        break;
        case AM_SCAN_PROGRESS_SCAN_END: {
            pT->mCurEv.mPercent = 100;
            pT->mCurEv.mLockedStatus = 0;
            pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_END;
            pT->sendEvent(pT->mCurEv);
        }
        break;
        case AM_SCAN_PROGRESS_SCAN_EXIT: {
            pT->mCurEv.mLockedStatus = 0;
            pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_EXIT;
            pT->sendEvent(pT->mCurEv);
        }
        break;
        case AM_SCAN_PROGRESS_ATV_TUNING: {
            pT->mCurEv.mFrequency = (int)evt->data;
            pT->mCurEv.mLockedStatus = 0;
            tmpFreq = (pT->mCurEv.mFrequency - pT->mCurScanStartFreq) / 1000000;
            pT->mCurEv.mPercent = tmpFreq * 100UL / ((pT->mCurScanEndFreq - pT->mCurScanStartFreq) / 1000000);
            pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
            pT->sendEvent(pT->mCurEv);
        }
        break;

        default:
            break;
        }
    } else if (event_type == AM_SCAN_EVT_SIGNAL) {
        AM_SCAN_DTVSignalInfo_t *evt = (AM_SCAN_DTVSignalInfo_t *)param;
        //pT->mCurEv.mprogramType = 0xff;
        pT->mCurEv.mFrequency = (int)evt->frequency;
        pT->mCurEv.mLockedStatus = (evt->locked ? 1 : 0);

        if (pT->mCurEv.mMode == FE_ANALOG && evt->locked)//trick here for atv new prog
            pT->mCurEv.mLockedStatus |= 0x10;

        pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
        if (pT->mCurEv.mMode != FE_ANALOG && evt->locked) {
            pT->mCurEv.mStrength = evt->strength;
            pT->mCurEv.mSnr = evt->snr;
        } else {
            pT->mCurEv.mStrength = 0;
            pT->mCurEv.mSnr = 0;
        }

        //if (pT->mCurEv.mMode == FE_ANALOG)
        pT->sendEvent(pT->mCurEv);

        pT->mCurEv.mLockedStatus &= ~0x10;
    }
}

void CTvScanner::tv_scan_reconnect_dmx_to_fend(int dmx_no, int fend_no)
{
    AM_DMX_Source_t src;

    if (AM_FEND_GetTSSource(fend_no, &src) == AM_SUCCESS) {
        LOGD("Set demux%d source to %d", dmx_no, src);
        AM_DMX_SetSource(dmx_no, src);
    } else {
        LOGD("Cannot get frontend ts source!!");
    }
}

int CTvScanner::getAtscChannelPara(int attennaType, Vector<sp<CTvChannel> > &vcp)
{
    switch (attennaType) { //region name should be remove to config file and read here
    case 1:
        CTvRegion::getChannelListByName("U.S.,ATSC Air", vcp);
        break;
    case 2:
        CTvRegion::getChannelListByName("U.S.,ATSC Cable Standard", vcp);
        break;
    case 3:
        CTvRegion::getChannelListByName("U.S.,ATSC Cable IRC", vcp);
        break;
    case 4:
        CTvRegion::getChannelListByName("U.S.,ATSC Cable HRC", vcp);
        break;
    default:
        return -1;
    }

    return 0;
}

void CTvScanner::sendEvent(ScannerEvent &evt)
{
    if (mpObserver) {
        if (evt.mType != ScannerEvent::EVENT_DTV_PROG_DATA) {
            evt.mAcnt = 0;//avoid invalid count confused the array.
            evt.mScnt = 0;
        }
        mpObserver->onEvent(evt);
    }
}

