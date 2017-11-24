#ifndef INCLUDED_DPSDK_CORE_PLAYBACK_H
#define INCLUDED_DPSDK_CORE_PLAYBACK_H

/** 开启平台录像
@param   IN    nPDLLHandle     SDK句柄
@param   IN    szCameraId      通道ID
@param   IN    streamType      实时码流类型
@param   IN    nTimeout        超时时长，单位毫秒
@return  函数返回错误类型，参考dpsdk_retval_e
@remark
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_StartPlatformReocrd(IN int32_t nPDLLHandle,
																  IN const char *szCameraId,
																  IN dpsdk_encdev_stream_e streamType = DPSDK_CORE_STREAM_MAIN,
																  IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 停止平台录像
@param   IN    nPDLLHandle     SDK句柄
@param   IN    szCameraId      通道ID
@param   IN    nTimeout        超时时长，单位毫秒
@return  函数返回错误类型，参考dpsdk_retval_e
@remark
*/ 
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_StopPlatformReocrd(IN int32_t nPDLLHandle,
																 IN const char *szCameraId,
																 IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );


/** 查询录像.
 @param   IN	nPDLLHandle		SDK句柄
 @param   IN	pQueryInfo		查询信息
 @param   OUT	nRecordCount	录像记录个数
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
   1、nRecordCount最大5000个记录
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_QueryRecord( IN int32_t nPDLLHandle, 
														   IN Query_Record_Info_t* pQueryInfo, 
														   OUT int32_t& nRecordCount, 
														   IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 通过码流类型查询录像.
 @param   IN	nPDLLHandle				SDK句柄
 @param   IN	pQueryInfo				查询信息
 @param   IN	nStreamType				码流类型
 @param   OUT	nRecordCount			录像记录个数
 @param   IN	nTimeout				超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
   1、nRecordCount最大5000个记录
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_QueryRecordByStreamType( IN int32_t nPDLLHandle, 
																		IN Query_Record_Info_t* pQueryInfo, 
																		IN dpsdk_stream_type_e nStreamType,
																		OUT int32_t& nRecordCount, 
																		IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 获取录像信息.
 @param   IN	nPDLLHandle		SDK句柄
 @param   OUT	pRecords		录像信息
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
   1、必须先查询后获取
   2、DPSDK_QueryRecord会返回记录个数,
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_GetRecordInfo( IN int32_t nPDLLHandle, 
															 INOUT Record_Info_t* pRecords );

/** 按文件请求录像流.
 @param	  IN	nPDLLHandle		SDK句柄
 @param   OUT	nPlaybackSeq	回放请求序号,作为后续操作标识  
 @param	  IN	pRecordInfo		录像信息 
 @param   IN    pFun			码流回调函数				
 @param   IN    pUser			码流回调用户参数
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_GetRecordStreamByFile( IN int32_t nPDLLHandle, 
																	 OUT int32_t& nPlaybackSeq, 
																	 IN Get_RecordStream_File_Info_t* pRecordInfo, 
																	 IN fMediaDataCallback pFun, 
																	 IN void* pUser, 
																	 IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 按时间请求录像流.
 @param	  IN	nPDLLHandle		SDK句柄
 @param   OUT	nPlaybackSeq	回放请求序号,作为后续操作标识  
 @param	  IN	pRecordInfo		录像信息 
 @param   IN    pFun			码流回调函数				
 @param   IN    pUser			码流回调用户参数
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_GetRecordStreamByTime( IN int32_t nPDLLHandle, 
																	 OUT int32_t& nPlaybackSeq, 
																	 IN Get_RecordStream_Time_Info_t* pRecordInfo, 
																	 IN fMediaDataCallback pFun, 
																	 IN void* pUser, 
																	 IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 根据码流类型按时间请求录像流.
 @param	  IN	nPDLLHandle		SDK句柄
 @param   OUT	nPlaybackSeq	回放请求序号,作为后续操作标识  
 @param	  IN	pRecordInfo		录像信息
 @param   IN	nStreamType		码流类型
 @param   IN    pFun			码流回调函数				
 @param   IN    pUser			码流回调用户参数
 @param   IN	nTimeout		超时时长，单位毫秒
 @param   IN	nTransMode		传输模式，1:TCP 0:UDP
 @param   IN	bBack			是否倒放
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_GetRecordStreamByStreamType( IN int32_t nPDLLHandle, 
																			OUT int32_t& nPlaybackSeq, 
																			IN Get_RecordStream_Time_Info_t* pRecordInfo,
									 										IN dpsdk_stream_type_e nStreamType,
																			IN fMediaDataCallback pFun, 
																			IN void* pUser, 
																			IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT,
																			IN int32_t nTransMode = 1,
																			IN bool bBack = false);

/** 暂停录像流.
 @param	  IN	nPDLLHandle		SDK句柄
 @param   IN	nPlaybackSeq	回放请求序号 
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_PauseRecordStreamBySeq( IN int32_t nPDLLHandle, 
																	  IN int32_t nPlaybackSeq, 
																	  IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 恢复录像流.
 @param	  IN	nPDLLHandle		SDK句柄
 @param   IN	nPlaybackSeq	回放请求序号 
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_ResumeRecordStreamBySeq( IN int32_t nPDLLHandle, 
																	   IN int32_t nPlaybackSeq, 
																	   IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 设置录像流速率.
 @param	  IN	nPDLLHandle		SDK句柄
 @param   IN	nPlaybackSeq	回放请求序号 
 @param   IN    nSpeed,         录像流回放速度
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_SetRecordStreamSpeed( IN int32_t nPDLLHandle, 
																	IN int32_t nPlaybackSeq, 
																	IN dpsdk_playback_speed_e nSpeed, 
																	IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 定位回放.
 @param	  IN	nPDLLHandle		SDK句柄
 @param   IN	nPlaybackSeq	回放请求序号 
 @param   IN    seekBegin,		定位起始值.文件模式时,是定位处的文件大小值;时间模式时,是定位处的时间值;
 @param   IN    seekEnd,		定位结束值.文件模式时,无意义;时间模式时,是期待的结束时间.
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark 
			seekBegin在文件模式下的计算方式可以是:(文件大小值)/100*(定位处相对文件的百分比)  
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_SeekRecordStreamBySeq(IN int32_t nPDLLHandle, 
																	IN int32_t nPlaybackSeq, 
																	IN uint64_t seekBegin, 
																	IN uint64_t seekEnd, 
																	IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 关闭录像流.
 @param	  IN	nPDLLHandle		SDK句柄
 @param   IN	nPlaybackSeq	回放请求序号 
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_CloseRecordStreamBySeq( IN int32_t nPDLLHandle, 
																	  IN int32_t nPlaybackSeq, 
																	  IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 关闭录像，同步等待服务应答.
 @param	  IN	nPDLLHandle		SDK句柄
 @param   IN	nPlaybackSeq	回放请求序号 
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_SynchroCloseRecordStreamBySeq( IN int32_t nPDLLHandle, 
																			  IN int32_t nPlaybackSeq, 
																			  IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 按通道关闭录像流.
 @param	  IN	nPDLLHandle		SDK句柄
 @param   IN	szCameraId   	通道编号 
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_CloseRecordStreamByCameraId( IN int32_t nPDLLHandle, 
																		   IN const char* szCameraId, 
																		   IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 按通道关闭录像流，同步等待服务应答.
 @param	  IN	nPDLLHandle		SDK句柄
 @param   IN	szCameraId   	通道编号 
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_SynchroCloseRecordStreamByCameraId( IN int32_t nPDLLHandle, 
																				IN const char* szCameraId, 
																				IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );



/** 手动启动设备录像
 @param	  IN	nPDLLHandle		SDK句柄
 @param const char* szDevId							设备ID
 @param const char* szSN							    互联编码
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_StartDeviceRecord( IN int32_t nPDLLHandle, 
																		  const char* szDevId, const char* szSN,
																		   IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 手动停止设备录像
 @param	  IN	nPDLLHandle		SDK句柄
 @param const char* szDevId							设备ID
 @param const char* szSN							    互联编码
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
 @remark  
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_StopDeviceRecord( IN int32_t nPDLLHandle, 
																		  const char* szDevId, const char* szSN,
																		   IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT );

/** 京东项目定制，根据历史视频文件ID查询录像信息
 @param   IN	nPDLLHandle		SDK句柄
 @param   IN	szFileId		历史视频文件ID
 @param   OUT	szCameraId		摄像头ID
 @param   OUT	begTime			开始时间
 @param   OUT	endTime			结束时间
 @param   OUT	szFilePath		历史视频文件路径
 @param   IN	nTimeout		超时时长，单位毫秒
 @return  函数返回错误类型，参考dpsdk_retval_e
*/
DPSDK_DLL_API int32_t DPSDK_CALL_METHOD DPSDK_QueryRecordByFileId( IN int32_t nPDLLHandle, 
																   IN const char* szFileId,
																   OUT char* szCameraId,
																   OUT uint64_t& begTime,
																   OUT uint64_t& endTime,
																   OUT char* szFilePath,
																   IN int32_t nTimeout = DPSDK_CORE_DEFAULT_TIMEOUT);
#endif


