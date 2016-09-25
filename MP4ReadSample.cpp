    /////  2. Identify the video & get its properties.  /////
    /////                                               /////
    /////////////////////////////////////////////////////////
    m_video_trId = MP4FindTrackId(m_hMP4File, 0, MP4_VIDEO_TRACK_TYPE, 0);
    if (m_video_trId != MP4_INVALID_TRACK_ID) {

        video_name = (char *) MP4GetTrackMediaDataName(m_hMP4File, m_video_trId);
        if (strcmp(video_name, "mp4v") == 0)
            m_video_codec = RAW_STRM_TYPE_M4V;        // MPEG4
        else if (strcmp(video_name, "s263") == 0)
            m_video_codec = RAW_STRM_TYPE_H263;        // H.263
        else if (strcmp(video_name, "avc1") == 0)
            m_video_codec = RAW_STRM_TYPE_H264RAW;    // H.264
        else
            m_video_codec = 4;    // Unknown

        // Timescale (ticks per second) and duration of the Video track
        m_video_timescale       = MP4GetTrackTimeScale(m_hMP4File, m_video_trId);
        m_video_duration        = MP4GetTrackDuration(m_hMP4File, m_video_trId);

        // Number of video samples, video frame rate
        m_video_num_samples     = MP4GetTrackNumberOfSamples(m_hMP4File, m_video_trId);
        m_video_framerate       = MP4GetTrackVideoFrameRate(m_hMP4File, m_video_trId);
        m_video_sample_max_size = MP4GetTrackMaxSampleSize(m_hMP4File, m_video_trId);

        GetVideoProfileAndSize(m_hMP4File, m_video_trId, m_video_codec,
                               &m_video_profile, &m_video_width, &m_video_height);

printf("/n m_video_framerate = (%f) !! m_video_duration = (%d), m_video_timescale = (%d)/n", m_video_framerate, m_video_duration, m_video_timescale);
    }
//这里因为头文件里面的音视频信息并不是准确的，经常会出错
//因而这里我们写了一个函数来专门确定我们的媒体属性
//GetVideoProfileAndSize
//视频的完了就取音频

    /////////////////////////////////////////////////////////
    /////                                               /////
    /////  3. Identify the audio & get its properties.  /////
    /////                                               /////
    /////////////////////////////////////////////////////////
    m_audio_trId = MP4FindTrackId(m_hMP4File, 0, MP4_AUDIO_TRACK_TYPE, 0);
    if (m_audio_trId != MP4_INVALID_TRACK_ID) {
        audio_name = (char *) MP4GetTrackMediaDataName(m_hMP4File, m_audio_trId);
        if (strcmp(audio_name, "mp4a") == 0)
            m_audio_codec = RAW_STRM_TYPE_M4A;            // MPEG4 AAC
        else if (strcmp(audio_name, "samr") == 0)
            m_audio_codec = RAW_STRM_TYPE_AMR_NB;        // AMR-NB
        else {
            printf("/n !! Unknown Audio (%s) !!/n", audio_name);
            m_audio_codec = 4;    // Unknown
        }

        // Timescale (ticks per second) and duration of the Video track
        m_audio_timescale       = MP4GetTrackTimeScale(m_hMP4File, m_audio_trId);
        m_audio_duration        = MP4GetTrackDuration(m_hMP4File, m_audio_trId);

        // Number of video samples, video width, video height, video frame rate
        m_audio_num_samples     = MP4GetTrackNumberOfSamples(m_hMP4File, m_audio_trId);
        m_audio_num_channels    = 2;//MP4GetTrackAudioChannels(m_hMP4File, m_audio_trId);
        m_audio_sample_max_size = MP4GetTrackMaxSampleSize(m_hMP4File, m_audio_trId);

printf("/n m_audio_trId = %d, m_audio_duration = (%d), m_audio_timescale = (%d) !!/n", m_audio_trId, m_audio_duration, m_audio_timescale);
    }
//最后函数返回
    return S_OK;
}

这里我们读取数据都是有一个sample_id，文件储存是按照一帧一帧的存储的，得到sample_id，就可以得到该帧的文件偏移，然后读取文件到buff
int CMP4ParserFilter::GetAudioFrame(unsigned char *pFrameBuf, unsigned int *pSize, int sample_id)
{
    int nRead;

    u_int8_t      *p_audio_sample;
    u_int32_t      n_audio_sample;

    int            b, ret;


    p_audio_sample = (u_int8_t *) pFrameBuf;
    n_audio_sample = *pSize;


    if ((sample_id <= 0) || (sample_id > m_audio_num_samples)) {
        RETAILMSG(1, (L"/n[CMP4ParserFilter::GetAudioFrame] /'sample_id/' is invalid. (sample_id = %d", sample_id));
        *pSize = 0;
        return -1;
    }

    /////////////////////
    //  MP4ReadSample  //
    /////////////////////
    b = MP4ReadSample(m_hMP4File, m_audio_trId, sample_id,
                      &p_audio_sample, &n_audio_sample,
                      NULL, NULL, NULL, NULL);
    if (!b) {
        *pSize = 0;
        return -1;
    }

//printf("/n  --- MP4ReadSample=%d", n_audio_sample);

    *pSize = nRead = n_audio_sample;

    return nRead;
}

int CMP4ParserFilter::GetVideoFrame(unsigned char *pFrameBuf, unsigned int *pSize, unsigned int *isIframe, int sample_id)
{
    int nRead;

    int            n_video_hdr;

    u_int8_t      *p_video_sample;
    u_int32_t      n_video_sample;

    int            b, ret;


    p_video_sample = (u_int8_t *) pFrameBuf;
    n_video_sample = *pSize;


    if ((sample_id <= 0) || (sample_id > m_video_num_samples)) {
        RETAILMSG(1, (L"/n[CMP4ParserFilter::GetVideoFrame] /'sample_id/' is invalid. (sample_id = %d", sample_id));
        *pSize = 0;
        return -1;
    }

    // if (sampleId == 1),
    // then "video stream header" is extracted and it is put in the buffer.
    if (sample_id == 1) {
        ret = GetVideoStreamHeader(m_hMP4File, m_video_trId, m_video_codec,
                                   pFrameBuf, &n_video_hdr);

        p_video_sample += n_video_hdr;
        n_video_sample -= n_video_hdr;
    }
    else
        n_video_hdr = 0;

    /////////////////////
    //  MP4ReadSample  //
    /////////////////////
    b = MP4ReadSample(m_hMP4File, m_video_trId, sample_id,
                      &p_video_sample, &n_video_sample,
                      NULL, NULL, NULL, NULL);
    if (!b) {
        *pSize = 0;
        return -1;
    }

    // if (codec == h.264), the first 4 bytes are the length octets.
    // They need to be changed to H.264 delimiter (00 00 00 01).
    if (m_video_codec == RAW_STRM_TYPE_H264RAW) {
        int h264_nal_leng;
        int nal_leng_acc = 0;
        do {
            h264_nal_leng = p_video_sample[0];
            h264_nal_leng = (h264_nal_leng << 8) | p_video_sample[1];
            h264_nal_leng = (h264_nal_leng << 8) | p_video_sample[2];
            h264_nal_leng = (h264_nal_leng << 8) | p_video_sample[3];
            memcpy(p_video_sample, h264_delimiter, 4);
            nal_leng_acc   += (h264_nal_leng + 4);
            p_video_sample += (h264_nal_leng + 4);
        } while (nal_leng_acc < n_video_sample);
    }


    *pSize = nRead = (n_video_hdr + n_video_sample);
    *isIframe  = MP4GetSampleSync(m_hMP4File, m_video_trId, sample_id);


    return nRead;
}