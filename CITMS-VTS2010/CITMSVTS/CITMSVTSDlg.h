
// CITMSVTSDlg.h : header file
//

#pragma once
#include "afxwin.h"

#include "Transcoding.h"
#include "RTMPSession.h"
#include "RecodeSound.h"

// CCITMSVTSDlg dialog
class CCITMSVTSDlg : public CDialogEx
{
// Construction
public:
	CCITMSVTSDlg(CWnd* pParent = NULL);	// standard constructor
	~CCITMSVTSDlg();

// Dialog Data
	enum { IDD = IDD_CITMSVTS_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnClose();
	afx_msg void OnBnClickedButtonRTMPStart();
	afx_msg void OnBnClickedButtonRTMPStop();
	CButton m_rtmp_start_btn;
	CButton m_rtmp_stop_btn;

public:
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);

//AudioSampler

public:
	afx_msg void OnBnClickedButtonDh();
	afx_msg void OnBnClickedButtonH264();
protected:
	CEdit m_HttpServiceStatus;
};
