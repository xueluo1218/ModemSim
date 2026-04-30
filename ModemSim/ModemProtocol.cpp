#include "StdAfx.h" 
#include "ModemProtocol.h"

// 常量定义
const DWORD UPLOAD_HEAD_MAGIC = 0xDDCCBBAA; // AA BB CC DD (Little Endian)
const WORD  CMD_DEV_ID        = 0x0046;
const WORD  CMD_TYPE_START    = 0x20F0;
const WORD  CMD_TYPE_STOP     = 0x2010;
const WORD  CMD_TYPE_CONFIG   = 0x0000;

CModemProtocol::CModemProtocol()
{
	m_vRecvBuf.reserve(2048);
	m_chRecvFrameCount = 0;
	m_chErrorFrameCount = 0;
	m_bStartLogging = FALSE; 
	m_bIsRecvParas = FALSE;
}

CModemProtocol::~CModemProtocol()
{
}

// ==========================================================================
//  部分 1: 接收解析 (处理粘包)
// ==========================================================================

int CModemProtocol::ParseRecvData(const BYTE* pData, int nLength)
{
	if (pData == NULL || nLength <= 0) return 0;

	// 1. 追加新数据到缓存
	m_vRecvBuf.insert(m_vRecvBuf.end(), pData, pData + nLength);

	int nCmdParsed = 0;

	// 2. 循环尝试切包
	while (m_vRecvBuf.size() > sizeof(CmdFrameHeader))
	{
		CmdFrameHeader* pHead = (CmdFrameHeader*)&m_vRecvBuf[0];

		// 2.1 检查包头 ID (0x0046)
		if (pHead->wDevID != CMD_DEV_ID)
		{
			// 错位了，移除1字节继续找
			m_vRecvBuf.erase(m_vRecvBuf.begin());
			continue;
		}

		// 2.2 获取长度 (Length包含 DataType + Payload)
		WORD wBodyLen = pHead->wLength;

		// 2.3 计算完整包长: Header(8字节) + Payload(wLength) + Checksum(2字节)
		// 注意：Header结构体里包含了 wLength，所以基准偏移是 8
		// CmdFrameHeader 大小是 10 字节 (包含 wDataType)，
		// 协议说 Length 包括 DataType(2) + EffectiveData.
		// 所以总长 = (sizeof(CmdFrameHeader)-2) + wBodyLen + 2(CS)
		// 简化公式： 8 (前导4字) + wBodyLen + 2 (CS)
		int nTotalPacketLen = 8 + wBodyLen + 2;

		// 2.4 检查缓存是否足够
		if ((int)m_vRecvBuf.size() < nTotalPacketLen)
		{
			break; // 数据不够，等待下次
		}

		// 2.5 提取并处理这一包
		ProcessOnePacket(&m_vRecvBuf[0], nTotalPacketLen);
		nCmdParsed++;

		// 2.6 移除已处理数据
		m_vRecvBuf.erase(m_vRecvBuf.begin(), m_vRecvBuf.begin() + nTotalPacketLen);
	}

	return nCmdParsed;
}

// 第5字：数据类型，0x20F0开始上传，0x0000为井下仪器命令
void CModemProtocol::ProcessOnePacket(const BYTE* pPacket, int nTotalLen)
{
	// 1. 验证设备ID (前2字节)
	if (pPacket[0] != 0x46 || pPacket[1] != 0x00) return;

	// 分析有几个0x46
	const CmdFrameHeader* pHead = (CmdFrameHeader*)pPacket;

	// 校验和验证
	// 范围：从数据长度之后的字开始计算，开始累加，数目为长度
	// ---------------------------------------------------------
	BYTE sum = 0;
	for (int i = 0; i < pHead->wLength; i++) {
		sum += pPacket[8+i];
	}
	WORD sumPacket = *(WORD*)(pPacket + 8 + pHead->wLength);
	WORD sumCalc = (WORD)sum;	// 高位自动补0
	if(1)//sumPacket == sumCalc)	// 校验和正确
	{		
		TRACE(_T("收到命令 0x%04X, 校验正确！\n"), pHead->wDataType); 

		// 定位有效数据起始位置 (Header之后)
		// Header是 10字节 (含DataType)
		const BYTE* pPayload = pPacket + sizeof(CmdFrameHeader);

		// 有效数据长度 = wLength - 2 (减去DataType的2字节)
		int nPayloadLen = pHead->wLength - 2; 

		if (pHead->wDataType == CMD_TYPE_START) // 0x20F0
		{
			// 解析速率等参数
			int nRate = 0;
			if (nPayloadLen >= 2) nRate = *(WORD*)pPayload;
			//OnCmdStartLog(nRate);

			// TODO: 设置标志位 
			m_bStartLogging = TRUE; 
			m_bIsRecvParas = FALSE;
		}
		else if (pHead->wDataType == CMD_TYPE_STOP) // 0x2010
		{
			//OnCmdStopLog();

			// TODO: 设置标志位 
			m_bStartLogging = FALSE; 
			m_bIsRecvParas = FALSE;
		}
		else if (pHead->wDataType == CMD_TYPE_CONFIG) // 0x0000
		{
			// 批处理命令解析
			// 格式: [Count(1 Word)] + [SubCmd1] + [SubCmd2] ...
			if (nPayloadLen >= 2)
			{
				WORD wCount = *(WORD*)pPayload; // 命令条数
				ToolSubCommand subCmd;
				OnCmdBatchConfig(wCount, pPayload + 2, nPayloadLen - 2, m_vecCmd);
			}
		}
	}			
	else{
		TRACE(_T("收到命令 0x%04X, 但是校验错，此命令不予处理！\n"), pHead->wDataType); 
	}
}

// 业务处理存根 (可以在外部继承或修改)
void CModemProtocol::OnCmdStartLog(int nRate) 
{ 
	TRACE(_T("收到开始上传命令, Rate=%d\n"), nRate); 
	// TODO: 设置标志位 
	m_bStartLogging = TRUE; 
}

void CModemProtocol::OnCmdStopLog() 
{ 
	TRACE(_T("收到上传测井命令\n")); 
	// TODO: 设置标志位 
	m_bStartLogging = FALSE; 
	m_bIsRecvParas  = FALSE;
}

void CModemProtocol::OnCmdBatchConfig(int nCmdCount, const BYTE* pParamData, int nParamTotalLen, ToolCommandManager& vecCmd)
{
	TRACE(_T("收到批处理命令，共 %d 条子命令\n"), nCmdCount);

	SubCommand cmd[MAX_COMMAND_NUMS];

	ZeroMemory(&cmd, sizeof(SubCommand) * MAX_COMMAND_NUMS);

	m_vecCmd.clear();

	// 这里可以进一步循环解析 pParamData 中的子命令
	int offset = 0;
	for(int i=0; i<nCmdCount; i++) {
		SubCommand* pSub = (SubCommand*)(pParamData + offset);
		offset += 6;			// 前部3个字
		int nParamCount(0);		// 实际的参数个数
		TRACE(_T("ToolIndex=%d CmdId=%04X, FunNum=%04X, ParamNum=%04X: "), i, pSub->wCmdID, pSub->wFuncNum, pSub->wParamNum);
		ToolSubCommand subcmd(pSub->wCmdID, pSub->wFuncNum, pSub->wParamNum);
		for(int j=0; j<pSub->wParamNum; j++){
			WORD param = *(WORD*)(pParamData + offset);
			offset += sizeof(WORD);
			if(param)
			{
				cmd[i].Params[j] = param;
				nParamCount++;
				TRACE(_T("%04X "), param);
				subcmd.addParam(param);
			}
		}
		m_vecCmd.addSubCommand(subcmd);

		//offset += sizeof(SubCommandHead) + pSub->wParamNum * 2;
		TRACE(_T("\n"));
	}

	// 接收到有效的参数帧
	if(m_vecCmd.size())
		m_bIsRecvParas = TRUE;
}
//
//// ==========================================================================
////  部分 2: 模拟数据生成 (Generate Packet)
//// ==========================================================================
//
//void CModemProtocol::GenerateSimPacket(BYTE* buffer, int& nOutLen)
//{
//	// 目标：构建 156 字节的数据包
//	int nOffset = 0;
//
//	// 1. 填充传输头 (14 Bytes)
//	UploadTransHeader* pHeader = (UploadTransHeader*)(buffer + nOffset);
//	pHeader->dwHead    = UPLOAD_HEAD_MAGIC; // AA BB CC DD
//	pHeader->wDevID    = 0x0046;
//	pHeader->dwTime    = GetTickCount();    // 模拟时间
//	pHeader->wLength   = 0x008E;            // 142 (Dec) = 2(Type) + 130(Body) + 10(Tail)
//	pHeader->wDataType = 0xC000;            // 上传数据类型
//
//	nOffset += sizeof(UploadTransHeader);
//
//	// 记录校验起始位置 (从 DataType 开始，即 nOffset - 2)
//	BYTE* pChecksumStart = buffer + nOffset - 2;
//
//	// 2. 填充有效数据体 (Payload)
//
//	// A. Modem本体数据块 (模拟 0x46/0xC802 部分)
//	// 00-01: AA 55 (Sync)
//	*(WORD*)(buffer + nOffset) = 0x55AA; nOffset += 2;
//	// 02-03: C8 02 (Addr)
//	*(WORD*)(buffer + nOffset) = 0x02C8; nOffset += 2;
//	// 04-05: 00 00
//	*(WORD*)(buffer + nOffset) = 0x0000; nOffset += 2;
//	// 06-07: Frame No
//	*(WORD*)(buffer + nOffset) = (WORD)(m_dwFrameCount++); nOffset += 2;
//	// 08-09: 67 4B (Sync Head Ret)
//	*(WORD*)(buffer + nOffset) = 0x4B67; nOffset += 2;
//	// 10-11: 50 00 (Cmd Ret)
//	*(WORD*)(buffer + nOffset) = 0x0050; nOffset += 2;
//
//	// 填充一些占位状态数据 (模拟 Hex 中的 00 88 ...)
//	memset(buffer + nOffset, 0, 40); // 简单清零或填充固定模式
//	buffer[nOffset] = 0x88;          // 模拟特定字节
//	nOffset += 40; // 调整偏移量，实际应根据Hex精确填充
//
//	// B. CAN 仪器 1 (0x87)
//	InstDataHead* pInst1 = (InstDataHead*)(buffer + nOffset);
//	pInst1->wSync = 0x55AA;
//	pInst1->wAddr = 0x0187;          // 87 01
//	pInst1->wLen  = 0x0011;          // 17字 = 34字节
//	nOffset += sizeof(InstDataHead);
//
//	// 填充 0x87 的数据内容 (28字节数据 + 2字节 DD EE)
//	// 注意: wLen包括Header后的所有内容吗？通常是数据体长度。
//	// Hex显示: 87 01 11 00 后跟了 34 字节。
//	memset(buffer + nOffset, 0x11, 26); // 模拟数据
//	nOffset += 26;
//	// 模拟 Hex 结尾: 10 10 20 20 20 30 DD EE
//	BYTE tail87[] = {0x10, 0x10, 0x20, 0x20, 0x20, 0x30, 0xDD, 0xEE};
//	memcpy(buffer + nOffset, tail87, 8);
//	nOffset += 8;
//
//	// C. CAN 仪器 2 (0x180)
//	InstDataHead* pInst2 = (InstDataHead*)(buffer + nOffset);
//	pInst2->wSync = 0x55AA;
//	pInst2->wAddr = 0x0180;          // 80 01
//	pInst2->wLen  = 0x000E;          // 14字 = 28字节
//	nOffset += sizeof(InstDataHead);
//
//	// 填充 0x180 数据
//	memset(buffer + nOffset, 0x74, 20); // 模拟数据
//	nOffset += 20;
//	// 模拟 Hex 结尾部分 (无 DD EE)
//	BYTE tail180[] = {0x32, 0x12, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00}; 
//	memcpy(buffer + nOffset, tail180, 8);
//	nOffset += 8;
//
//	// 3. 填充包尾 (10 Bytes: 11...55)
//	BYTE finalTail[] = {0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 
//		0x44, 0x44, 0x55, 0x55};
//	memcpy(buffer + nOffset, finalTail, 10);
//	nOffset += 10;
//
//	// 4. 计算并填充校验和 (2 Bytes)
//	// 校验范围：DataType(2) + 有效数据 + 包尾
//	int nCheckLen = nOffset - (sizeof(UploadTransHeader) - 2); // 当前位置 - DataType起始位置
//	WORD wSum = CalcChecksum(pHeader->wDataType, pChecksumStart + 2, nCheckLen - 2);
//
//	*(WORD*)(buffer + nOffset) = wSum;
//	nOffset += 2;
//
//	nOutLen = nOffset; // 应该等于 156
//}

WORD CModemProtocol::CalcChecksum(const BYTE* pData, int nLen)
{
	BYTE sum = 0;

	// 加数据
	for (int i = 0; i < nLen; i++)
		sum += pData[i];

	// 返回 (高字节为0)
	return (WORD)sum;	
}

// 带入每个仪器的数据，实现组包
void CModemProtocol::GenerateSimPacket(BYTE* buffer, int& nOutLen, const std::vector<SimInstrumentData>& vInstList)
{
	int nOffset = 0;

	// ---------------------------------------------------------
	// 1. 填充传输头预留位置 (12字节，稍后回填长度)
	// ---------------------------------------------------------
	UploadTransHeader* pHeader = (UploadTransHeader*)(buffer + nOffset);
	pHeader->dwHead    = 0xDDCCBBAA;		// 总帧头
	pHeader->wDevID    = 0x0046;			// 遥测MODEM IP地址
	pHeader->dwTime    = GetTickCount();	// 时间戳
	pHeader->wLength   = 0;					// 注意：wLength 暂时不知道，最后填
	pHeader->wDataType = 0xC000;			// 0xC000 - 上传的是数据；0xA0XX - 命令应答
	nOffset = sizeof(UploadTransHeader);	// 结构共14字节

	// 记录校验和计算的起点 (从 DataType 开始)
	int nPayloadStart = 12;					// DataType计算在校验和以内

	// ---------------------------------------------------------
	// 2. 填充 Modem 本体数据 (0x46 / 0xC802) - 总是存在
	// ---------------------------------------------------------
	*(WORD*)(buffer + nOffset) = 0x55AA; nOffset += 2;			// 00-01: AA 55
	*(WORD*)(buffer + nOffset) = 0x02C8; nOffset += 2;			// 02-03: C8 02 (Modem内部地址)
	*(WORD*)(buffer + nOffset) = 0x0000; nOffset += 2;			// 04-05: 保留
	*(buffer + nOffset) = (m_chRecvFrameCount++); nOffset++;	// 6: 接收数据帧号++
	*(buffer + nOffset) = (m_chErrorFrameCount); nOffset++;		// 7: 错误帧号
	*(WORD*)(buffer + nOffset) = 0x4B67; nOffset += 2;			// 8-9: 以下参看表6：返回第1个命令字：同步头
	*(WORD*)(buffer + nOffset) = 0x0050; nOffset += 2;			// 10-11: 标识符：网络模块工作周期

	// 以下20个字，40字节
	*(WORD*)(buffer + nOffset) = 0x0088; nOffset += 2;			// 12-13: 返回第3个命令字：电缆状态字
	*(WORD*)(buffer + nOffset) = 0x0044; nOffset += 2;			// 14-15: 返回第3个命令字：总线状态字（0x0040+2*仪器个数）
	*(WORD*)(buffer + nOffset) = 0x0000; nOffset += 2;			// 16-17: 当前下发命令的总长度
	*(WORD*)(buffer + nOffset) = 0x1100; nOffset += 2;			// 18-19: 上传数据字的总长度
	*(WORD*)(buffer + nOffset) = 0x0000; nOffset += 2;			// 20-21: 当前井下接收命令CRC结果，0正确，1错误
	*(WORD*)(buffer + nOffset) = 0x0000; nOffset += 2;			// 22-23: 前端中断方式，0不等前端机中断，1等中断
	*(WORD*)(buffer + nOffset) = 0x0000; nOffset += 2;			// 24-25: 当前上传数据出错标志。0无错，1有错
	*(WORD*)(buffer + nOffset) = 0x0000; nOffset += 2;			// 26-27: 大帧出错数量报告
	*(WORD*)(buffer + nOffset) = 0x0000; nOffset += 2;			// 28-29: 小帧出错数量报告
	*(WORD*)(buffer + nOffset) = 0x0000; nOffset += 2;			// 30-31: 未定义
	*(WORD*)(buffer + nOffset) = 0x0001; nOffset += 2;			// 33-34: 传输方式选择；1：430K模式
	*(WORD*)(buffer + nOffset) = 0x0000; nOffset += 2;			// 35-36: 传输速率选择
	for(int i=0; i<6; i++)				// 00 00 00 00 00 00 00 00 00 00 00 00 12字节未定义
	{
		*(WORD*)(buffer + nOffset) = 0x0000; nOffset += 2;	
	}
	*(WORD*)(buffer + nOffset) = 0x0000; nOffset += 2;			// 00 00 AD增益参数
	*(WORD*)(buffer + nOffset) = 0x0000; nOffset += 2;			// 00 00 DA增益参数


	// ---------------------------------------------------------
	// 3. 动态填充仪器列表
	// ---------------------------------------------------------
	for (size_t i = 0; i < vInstList.size(); i++)
	{
		const SimInstrumentData& inst = vInstList[i];

		// 3.1 写入通用仪器头 (AA 55 + ID + Len)
		InstDataHead* pInstHead = (InstDataHead*)(buffer + nOffset);
		pInstHead->wSync = 0x55AA;
		pInstHead->wAddr = inst.wID;

		// 计算字长度 (Bytes / 2)
		// 注意：协议要求偶数字节。如果是奇数，需要补0
		int nDataBytes = (int)inst.vData.size();
		if (nDataBytes % 2 != 0) nDataBytes++;		// 偶数对齐

		pInstHead->wLen = (WORD)(nDataBytes / 2);	// 上传长度为字

		nOffset += sizeof(InstDataHead); 

		// 3.2 写入具体数据
		if (!inst.vData.empty())
		{
			memcpy(buffer + nOffset, &inst.vData[0], inst.vData.size());
			nOffset += inst.vData.size();
		}

		// 不是最后一种仪器，则加帧尾0xDD 0xEE
		if (i != vInstList.size() - 1)
		{
			*(WORD*)(buffer + nOffset) = 0xEEDD; nOffset += 2;		
		}

		// 3.3 补齐奇数位 (如果原数据是奇数)
		if (inst.vData.size() % 2 != 0)
		{
			buffer[nOffset] = 0x00; 
			nOffset++;
		}
	}

	// ---------------------------------------------------------
	// 6. 回填长度字段 (wLength)：数据类型+有效数据的长度，不考虑后续的两个校验和
	// ---------------------------------------------------------
	// wLength = DataType(2) + 有效数据
	// 当前 nOffset 指向有效数据包尾之后。
	// Header大小是 14 (包含DataType)。 Payload从 14 开始。
	// 计算总长度：nOffset
	// 协议中的 wLength = (nOffset - 14) + 2
	// 即：wLength = 总字节数 - (Header前导 12 字节)
	pHeader->wLength = (WORD)(nOffset - 12);

	// ---------------------------------------------------------
	// 4. 有效数据之后，计算第一个校验和（附加在有效数据之后）
	// 范围：数据类型+有效数据（Modem本体 + 所有仪器数据）
	// ---------------------------------------------------------
	int nPayloadEnd = nOffset;
	BYTE sum1 = 0;
	for (int i = nPayloadStart; i < nPayloadEnd; i++) {
		sum1 += buffer[i];
	}
	*(WORD*)(buffer + nOffset) = (WORD)sum1; // 高位自动补0
	nOffset += 2;

	// ---------------------------------------------------------
	// 5. 填充包尾 (10 Bytes)
	// ---------------------------------------------------------
	int nTailStart = nOffset;
	BYTE finalTail[] = {0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44, 0x44, 0x55, 0x55};
	memcpy(buffer + nOffset, finalTail, 10);
	nOffset += 10;

	// ---------------------------------------------------------
	// 6. 校验和 2 计算(最后 2 字节)
	// ---------------------------------------------------------
	// 校验范围：数据类型+有效数据 + (10字节帧尾)
	// 经测试，必须跳过第一个校验和的 2 个字节
	BYTE sum2 = 0;
	// 加前半部分
	for (int i = nPayloadStart; i < nPayloadEnd; i++) {
		sum2 += buffer[i];
	}
	// 加帧尾部分
	for (int i = nTailStart; i < nTailStart + 10; i++) {
		sum2 += buffer[i];
	}
	*(WORD*)(buffer + nOffset) = (WORD)sum2; 
	nOffset += 2;

	// 7. 回填传输头的总长度 (Len 字段)
	// 长度 = nOffset - 14 (不含传输头本身)
	pHeader->wLength = (WORD)(nOffset - sizeof(UploadTransHeader));

	nOutLen = nOffset;
}

#define byte0(var)      *((unsigned char *) &var)
#define byte1(var)      *((unsigned char *) &var + 1)

WORD CalcChecksum(WORD* pData, int nWordLen)
{
	DWORD sum = 0;

	// 加数据
	for (int i = 0; i < nWordLen; i++)
		sum += pData[i];

	// 返回 (高字节为0)
	return (sum & 0xFF);
}