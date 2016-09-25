//
#pragma once  
#include "mp4v2.h"
#include <Windows.h>
#pragma comment(lib,"libmp4v2.lib")
//
#define  _NALU_SPS_  0
#define  _NALU_PPS_  1
#define  _NALU_I_    2
#define  _NALU_P_    3
//
class CMp4Encoder
{
public:
	CMp4Encoder();
	bool InitMp4Encoder();
	void Mp4VEncode(BYTE*,int);
	void Mp4AEncode(BYTE*,int);
	void CloseMp4Encoder();
//----------------------------------------------------------- Attributes
private:
	int m_vWidth,m_vHeight,m_vFrateR,m_vTimeScale;
	MP4FileHandle m_mp4FHandle;
	MP4TrackId m_vTrackId,m_aTrackId;
	double m_vFrameDur;										  // 通过audio frame计算的两个视频帧间的间隔，单位为 1s/44100
};