//
#include "CMp4Encoder.h"
//
//
CMp4Encoder::CMp4Encoder()
	: m_vWidth(320),
	  m_vHeight(240),
	  m_vFrateR(10),
	  m_vTimeScale(90000),
	  m_mp4FHandle(NULL),
	  m_aTrackId(MP4_INVALID_TRACK_ID),
	  m_vTrackId(MP4_INVALID_TRACK_ID),
	  m_vFrameDur(3000)
{
}

bool CMp4Encoder::InitMp4Encoder()
{
	//------------------------------------------------------------------------------------- file handle
	m_mp4FHandle = MP4Create("c:\\lsh.mp4");
	if (m_mp4FHandle == MP4_INVALID_FILE_HANDLE){
		MessageBoxA(NULL,"mp4fileHandle Error!","ERROR",MB_OK);
		return false;  
	}
	MP4SetTimeScale(m_mp4FHandle, m_vTimeScale);
	//------------------------------------------------------------------------------------- audio track
	m_aTrackId = MP4AddAudioTrack(m_mp4FHandle, 44100, 1024, MP4_MPEG4_AUDIO_TYPE);
	if (m_aTrackId == MP4_INVALID_TRACK_ID){
		MessageBoxA(NULL,"AudioTrack Error!","ERROR",MB_OK);
		return false;
	}
	//
	MP4SetAudioProfileLevel(m_mp4FHandle, 0x2);
	BYTE aacConfig[2] = {18,16};                                                            // 参照博客
	MP4SetTrackESConfiguration(m_mp4FHandle,m_aTrackId,aacConfig,2);
	//-------------------------------------------------------------------------------------
	return true;
}


//------------------------------------------------------------------------------------------------- Mp4Encode说明
// 【x264编码出的NALU规律】
// 第一帧 SPS【0 0 0 1 0x67】 PPS【0 0 0 1 0x68】 SEI【0 0 1 0x6】 IDR【0 0 1 0x65】
// p帧      P【0 0 0 1 0x41】
// I帧    SPS【0 0 0 1 0x67】 PPS【0 0 0 1 0x68】 IDR【0 0 1 0x65】
// 【mp4v2封装函数MP4WriteSample】
// 此函数接收I/P nalu,该nalu需要用4字节的数据大小头替换原有的起始头，并且数据大小为big-endian格式
//-------------------------------------------------------------------------------------------------

void CMp4Encoder::Mp4VEncode(BYTE* _naluData,int _naluSize)
{
	int index = -1;
	//
	if(_naluData[0]==0 && _naluData[1]==0 && _naluData[2]==0 && _naluData[3]==1 && _naluData[4]==0x67){
		index = _NALU_SPS_;
	}
	if(index!=_NALU_SPS_ && m_vTrackId==MP4_INVALID_TRACK_ID){
		return;
	}
	if(_naluData[0]==0 && _naluData[1]==0 && _naluData[2]==0 && _naluData[3]==1 && _naluData[4]==0x68){
		index = _NALU_PPS_;
	}
	if(_naluData[0]==0 && _naluData[1]==0 && _naluData[2]==1 && _naluData[3]==0x65){
		index = _NALU_I_;
	}
	if(_naluData[0]==0 && _naluData[1]==0 && _naluData[2]==0 && _naluData[3]==1 && _naluData[4]==0x41){
		index = _NALU_P_;
	}
	//
	switch(index){
	case _NALU_SPS_:         
		if(m_vTrackId == MP4_INVALID_TRACK_ID){
			m_vTrackId = MP4AddH264VideoTrack  
				(m_mp4FHandle,   
				m_vTimeScale,   
				m_vTimeScale / m_vFrateR,  
				m_vWidth,     // width  
				m_vHeight,    // height  
				_naluData[5], // sps[1] AVCProfileIndication  
				_naluData[6], // sps[2] profile_compat  
				_naluData[7], // sps[3] AVCLevelIndication  
				3);           // 4 bytes length before each NAL unit  
			if (m_vTrackId == MP4_INVALID_TRACK_ID)  {  
				MessageBoxA(NULL,"add video track failed.\n","ERROR!",MB_OK);  
				return;  
			} 
			MP4SetVideoProfileLevel(m_mp4FHandle, 1); //  Simple Profile @ Level 3
		}  
		MP4AddH264SequenceParameterSet(m_mp4FHandle,m_vTrackId,_naluData+4,_naluSize-4);  
		//
		break;
	case _NALU_PPS_:  
		MP4AddH264PictureParameterSet(m_mp4FHandle,m_vTrackId,_naluData+4,_naluSize-4);  
		//
		break;
	case _NALU_I_: 
		{
			BYTE* IFrameData = new BYTE[_naluSize+1];
			//
			IFrameData[0] = (_naluSize-3) >>24;  
			IFrameData[1] = (_naluSize-3) >>16;  
			IFrameData[2] = (_naluSize-3) >>8;  
			IFrameData[3] = (_naluSize-3) &0xff;  
			//
			memcpy(IFrameData+4,_naluData+3,_naluSize-3);
			if(!MP4WriteSample(m_mp4FHandle, m_vTrackId, IFrameData, _naluSize+1, m_vFrameDur/44100*90000, 0, 1)){  
				return;  
			}  
			//
			m_vFrameDur = 0;
			delete [] IFrameData;         
			//
			break;
		}
	case _NALU_P_:
		{
			_naluData[0] = (_naluSize-4) >>24;  
			_naluData[1] = (_naluSize-4) >>16;  
			_naluData[2] = (_naluSize-4) >>8;  
			_naluData[3] = (_naluSize-4) &0xff; 
			//
			if(!MP4WriteSample(m_mp4FHandle, m_vTrackId, _naluData, _naluSize, m_vFrameDur/44100*90000, 0, 1)){  
				return;  
			}
			//
			m_vFrameDur = 0;
			//
			break;
		}
	}
}

void CMp4Encoder::Mp4AEncode(BYTE* _aacData,int _aacSize)
{
	if(m_vTrackId == MP4_INVALID_TRACK_ID){
		return;
	}
	MP4WriteSample(m_mp4FHandle, m_aTrackId, _aacData, _aacSize , MP4_INVALID_DURATION, 0, 1);
	//
	m_vFrameDur += 1024;
}

void CMp4Encoder::CloseMp4Encoder()
{
	if(m_mp4FHandle){  
		MP4Close(m_mp4FHandle);  
		m_mp4FHandle = NULL;  
	}
}
