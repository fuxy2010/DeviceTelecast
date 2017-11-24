// RecodeSound.cpp : 实现文件
//

#include "stdafx.h"
#include "RecodeSound.h"
#include "scheduleserver.h"

using namespace ScheduleServer;

IMPLEMENT_DYNCREATE(CRecodeSound, CWinThread)

CRecodeSound::CRecodeSound()//(CDialog *pDlg /* = NULL */)
{
	//m_pDlg = pDlg;
	//m_RecodeLog.Open(TEXT("recording.log"), CFile::modeCreate | CFile::modeWrite);
	//m_RecodeLog.WriteString(TEXT("In the Recordsound Constructor\n"));

	m_IsRecoding = FALSE; //初始还未开始录制
	m_IsAllocated = 0;//初始还未分配buffer

	PreCreateHeader();  //分配buffer

	memset(&m_WaveFormatEx, 0, sizeof(m_WaveFormatEx));
	m_WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;//声音格式为PCM		
	m_WaveFormatEx.nChannels = 1;	//采样声道数，对于单声道音频设置为1，立体声设置为2
	m_WaveFormatEx.wBitsPerSample = 16;//采样比特  8bits/次
	m_WaveFormatEx.cbSize = 0;//一般为0
	m_WaveFormatEx.nSamplesPerSec = 44100; //采样率 16000 次/秒
	m_WaveFormatEx.nBlockAlign = 2; //一个块的大小，采样bit的字节数乘以声道数
	m_WaveFormatEx.nAvgBytesPerSec = 88200; //每秒的数据率，就是每秒能采集多少字节的数据
}
CRecodeSound::~CRecodeSound()
{
}

BOOL CRecodeSound::InitInstance()
{
	// TODO: 在此执行任意逐线程初始化
	return TRUE;
}

int CRecodeSound::ExitInstance()
{
	// TODO: 在此执行任意逐线程清理
	return CWinThread::ExitInstance();
}


void CRecodeSound::PreCreateHeader()
{
	for(int i=0;i<MAXRECBUFFER;i++)
		m_RecHead[i]=CreateWaveHeader();
	m_IsAllocated = 1;
}

LPWAVEHDR  CRecodeSound::CreateWaveHeader()
{
	LPWAVEHDR lpHdr = new WAVEHDR;

	if(lpHdr==NULL)
	{
		//m_RecodeLog.WriteString(TEXT("\n Unable to allocate the memory"));
		return NULL;
	}

	ZeroMemory(lpHdr, sizeof(WAVEHDR));
	char* lpByte = new char[RECBUFFER];//m_WaveFormatEx.nBlockAlign*SOUNDSAMPLES)];

	if(lpByte==NULL)
	{
		//m_RecodeLog.WriteString(TEXT("\n Unable to allocate the memory"));
		return NULL;
	}
	lpHdr->lpData =  lpByte;
	lpHdr->dwBufferLength =RECBUFFER;   // (m_WaveFormatEx.nBlockAlign*SOUNDSAMPLES);
	return lpHdr;

}
void CRecodeSound::OnStartRecording(WPARAM wParam, LPARAM lParam)
{
	//m_RecodeLog.WriteString(TEXT("In OnStartrecording\n"));

	if(m_IsRecoding) //如果已经开启采集，则直接返回
		return ;//FALSE;

	//开启音频采集
	MMRESULT mmReturn = ::waveInOpen( &m_hRecord, WAVE_MAPPER, &m_WaveFormatEx, ::GetCurrentThreadId(), 0, CALLBACK_THREAD);

	//Error has occured while opening device

	if(mmReturn != MMSYSERR_NOERROR ) //打开采集失败
	{
		displayError(mmReturn,"Open");
		return ;//FALSE;
	}		

	if(mmReturn == MMSYSERR_NOERROR )
	{
		//将准备好的buffer提供给音频输入设备
		for(int i=0; i < MAXRECBUFFER ; i++)
		{
			//准备一个bufrer给输入设备
			mmReturn = ::waveInPrepareHeader(m_hRecord,m_RecHead[i], sizeof(WAVEHDR));
			//发送一个buffer给指定的输入设备，当buffer填满将会通知程序
			mmReturn = ::waveInAddBuffer(m_hRecord, m_RecHead[i], sizeof(WAVEHDR));
		}
		//开启指定的输入采集设备
		mmReturn = ::waveInStart(m_hRecord);

		if(mmReturn!=MMSYSERR_NOERROR )  //开始采集失败
			displayError(mmReturn,"Start");
		else
			m_IsRecoding = TRUE;
	}
}

void CRecodeSound::OnStopRecording(WPARAM wParam, LPARAM lParam)
{

	//m_RecodeLog.WriteString(TEXT("\nIn the onstop recording"));
	MMRESULT mmReturn = 0;

	if(!m_IsRecoding) //FALSE
		return ;

	//停止音频采集
	mmReturn = ::waveInStop(m_hRecord);

	//To get the remaining sound data from buffer
	//Reset will call OnSoundData function	

	if(!mmReturn) //停止采集成功，立即重置设备,重置设备将会导致所有的buffer反馈给程序
	{
		m_IsRecoding = FALSE;
		mmReturn = ::waveInReset(m_hRecord);  //重置设备
	}

	Sleep(500); //等待一段时间，使buffer反馈完成

	if(!mmReturn) //重置设备成功，立即关闭设备
		mmReturn = ::waveInClose(m_hRecord); //关闭设备

	return ;//mmReturn;

}

unsigned long time1 = 0;
unsigned long time2 = 0;
unsigned long duration = 0;
unsigned long frames = 0;
void CRecodeSound::OnSoundData(WPARAM wParam, LPARAM lParam)
{
	//m_RecodeLog.WriteString(TEXT("\nIn the onsound data"));

	if(m_IsRecoding==FALSE) //如果当前不在采集状态
		return ;//FALSE;

	LPWAVEHDR lpHdr = (LPWAVEHDR)lParam;

	if(lpHdr->dwBytesRecorded==0 || lpHdr==NULL)
		return ;//ERROR_SUCCESS;

	//使采集过程，知道此buffer已经沾满，不能再填充
	::waveInUnprepareHeader(m_hRecord, lpHdr, sizeof(WAVEHDR));

	{
		time2 = timeGetTime();

		duration += time2 - time1;
		frames += 1;

		if(100 == frames)
		{
			//TRACE("\nAvg %d", duration / 100);

			duration = 0;
			frames = 0;
		}
		time1 = time2;
	}

	/*//将采集到的声音发送给播放线程
	if(((CVideoPlayDlg *)m_pDlg)->m_pPlaySound != NULL)
		((CVideoPlayDlg *)m_pDlg)->m_pPlaySound->PostThreadMessage(WM_PLAYSOUND_PLAYBLOCK, lpHdr->dwBytesRecorded, (LPARAM)lpHdr->lpData);*/
	
	// Send recorded audio to remote host...
	/*
	if(lpHdr->lpData!=NULL )
		( (CVideoNetDlg*) dlg )->daudio.SendAudioData((unsigned char *)lpHdr->lpData,lpHdr->dwBytesRecorded);
	*/
	if(RECBUFFER == lpHdr->dwBytesRecorded)
	{
		//插入语音数据到队列中
		CUserAgent* ua = SINGLETON(CScheduleServer).fetch_ua(123);

		if(NULL != ua && true == ua->_start_hls)
		{
			ua->add_sample_audio_packet(lpHdr->lpData, lpHdr->dwBytesRecorded);
		}
	}

	if(m_IsRecoding)
	{
		//重新将buffer恢复到准备填充状态
		::waveInPrepareHeader(m_hRecord, lpHdr, sizeof(WAVEHDR));
		::waveInAddBuffer(m_hRecord, lpHdr, sizeof(WAVEHDR));
	}
}

void CRecodeSound::OnEndThread(WPARAM wParam, LPARAM lParam)
{
	//m_RecodeLog.WriteString(TEXT("\nIN the onendthread"));

	if(m_IsRecoding)
		OnStopRecording(0,0);

	::PostQuitMessage(0);
	return ;//TRUE;
}

void CRecodeSound::displayError(int mmReturn,char errmsg[])
{
	TCHAR errorbuffer[MAX_PATH];
	TCHAR errorbuffer1[MAX_PATH];

	waveInGetErrorText(mmReturn, errorbuffer, MAX_PATH);
	wsprintf(errorbuffer1, TEXT("RECORD: %s : %x : %s") ,errmsg, mmReturn, errorbuffer);
	AfxMessageBox(errorbuffer1);  
}

BEGIN_MESSAGE_MAP(CRecodeSound, CWinThread)
	ON_THREAD_MESSAGE(MM_WIM_DATA, OnSoundData)
	ON_THREAD_MESSAGE(WM_RECORDSOUND_STARTRECORDING, OnStartRecording)
	ON_THREAD_MESSAGE(WM_RECORDSOUND_STOPRECORDING, OnStopRecording)
	ON_THREAD_MESSAGE(WM_RECORDSOUND_ENDTHREAD, OnEndThread)
END_MESSAGE_MAP()


// CRecodeSound 消息处理程序
