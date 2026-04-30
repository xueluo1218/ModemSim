#pragma once
#include <vector>
#include <cstdint>

// 使用1字节对齐，确保结构体与协议二进制流一致
#pragma pack(push, 1)

// ========================================================
// 1. 基础定义
// ========================================================
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;

// PC下发命令的通用头 (PC -> Modem)
struct CmdFrameHeader
{
	WORD wDevID;        // 0x0046
	WORD wTime[2];      // 时间
	WORD wLength;       // 长度 (包含 DataType + Data)
	WORD wDataType;     // 数据类型 (0x0000:命令, 0x20F0:开始, 0x2010:停止)
};

// 发送给井下仪器的命令
#define MAX_COMMAND_NUMS 10				// 一次下发的命令条数
#define MAX_COMMAND_PARAM_NUMS 17		// 每条命令所带参数个数

// 批处理命令的子命令头
typedef struct
{
	WORD wCmdID;        // Command_ID
	WORD wFuncNum;      // Function_Num
	WORD wParamNum;     // 参数个数 N
	// 后面紧跟 N * WORD 的参数
	WORD Params[MAX_COMMAND_PARAM_NUMS];     // 参数（实际下发的命令）
}SubCommand;

// 最多一次可发10条命令
//class 
//{
//	WORD SubCommand[MAX_COMMAND_NUMS];
//} Tool_Command_Package;		// 命令包
//

// ========================================================
// 2. 上传数据定义 (Modem -> PC)
// ========================================================

// 上传数据的传输头 (自定义外壳 AA BB CC DD)
struct UploadTransHeader
{
	DWORD dwHead;       // 0xAABBCCDD
	WORD  wDevID;       // 0x0046
	DWORD dwTime;       // 时间戳
	WORD  wLength;      // 长度 (0x008E = 142)
	WORD  wDataType;    // 0xC000=上传数据标识，0xA0XX=命令应答
};

// 内部仪器数据头 (AA 55 ...)
struct InstDataHead
{
	WORD wSync;         // 0x55AA (内存中 AA 55)
	WORD wAddr;         // 仪器地址
	WORD wLen;          // 长度 (字单位)
	// 后面紧跟数据
};

// 定义单个仪器的模拟数据包
struct SimInstrumentData
{
	WORD wID;                  // 仪器ID (例如 0x0187, 0x0180)
	std::vector<BYTE> vData;   // 仪器数据体 (必须包含该仪器特有的帧尾，如 DD EE)

	// 构造函数方便使用
	SimInstrumentData(WORD id) : wID(id) {}

	// 辅助函数：添加数据
	void AddBytes(const BYTE* p, int len) {
		vData.insert(vData.end(), p, p + len);
	}
};

#pragma pack(pop)


// 解析上位机命令
// 一维数组，定义单条指令，用于取代上述一条命令SubCommand
class ToolSubCommand
{
	WORD wCmdID;        // Command_ID
	WORD wFuncNum;      // Function_Num
	WORD wParamNum;     // 参数个数 N，实际没什么用，只是为了兼容原有协议
	// 后面紧跟 N * WORD 的参数
	//WORD Params[MAX_COMMAND_PARAM_NUMS];     /*参数（实际下发的命令）*/
	std::vector <WORD> vecParam;	

public:
	ToolSubCommand(WORD cmdID = 0, WORD funcNum = 0, WORD paramNum = 0) 
		: wCmdID(cmdID), wFuncNum(funcNum), wParamNum(paramNum){}

	// 获取参数（可修改）
	std::vector<WORD>& getParams() { return vecParam; }
	const std::vector<WORD>& getParams() const { return vecParam; }

	// 清空参数
	void clearParams() { vecParam.clear(); }

	// 添加参数
	void addParam(WORD param) { vecParam.push_back(param); }

	// 序列化（用于网络传输）
	std::vector<uint8_t> serialize() const {
		std::vector<uint8_t> buffer;
		uint16_t param_count = static_cast<uint16_t>(vecParam.size());
		size_t total_size = sizeof(wCmdID) + sizeof(wFuncNum) 
			+ sizeof(param_count) + param_count * sizeof(WORD);

		buffer.resize(total_size);
		uint8_t* ptr = buffer.data();

		memcpy(ptr, &wCmdID, sizeof(wCmdID));
		ptr += sizeof(wCmdID);

		memcpy(ptr, &wFuncNum, sizeof(wFuncNum));
		ptr += sizeof(wFuncNum);

		memcpy(ptr, &param_count, sizeof(param_count));
		ptr += sizeof(param_count);

		if (!vecParam.empty()) {
			memcpy(ptr, vecParam.data(), vecParam.size() * sizeof(WORD));
		}

		return buffer;
	}
};  

// 二维数组：多个ToolSubCommand的集合
class ToolCommandManager {
private:
	std::vector<ToolSubCommand> subCommands; // 存储对象本身

public:
	// 方法1：添加已构造的对象（拷贝）
	void addSubCommand(const ToolSubCommand& cmd) {
		subCommands.push_back(cmd); // 触发拷贝构造
	}

	// 方法2：移动添加（C++11起，更高效）
	void addSubCommand(ToolSubCommand&& cmd) {
		subCommands.push_back(std::move(cmd)); // 移动构造，避免拷贝
	}

	// 方法3：就地构造（推荐：最高效，直接构造在容器内）
	//template<typename... Args>
	//void emplaceSubCommand(Args&&... args) {
	//	subCommands.emplace_back(std::forward<Args>(args)...);
	//}

	void clear(){subCommands.clear();}
	size_t size(){return subCommands.size();}
	ToolSubCommand GetSubCommand(int i){return subCommands[i];}
};

// ========================================================
// 3. 协议处理类定义
// ========================================================
class CModemProtocol
{
public:
	CModemProtocol();
	virtual ~CModemProtocol();

	// ----------------------------------------------------
	// 接口 1: 解析接收到的 PC 数据
	// ----------------------------------------------------
	// 将 Socket 接收到的原始数据喂给此函数
	// 返回值: 解析出的完整命令数量
	int ParseRecvData(const BYTE* pData, int nLength);

	// 开始上传/结束上传
	BOOL GetLoggingState(){return m_bStartLogging & m_bIsRecvParas;}
	// 井下仪器配置参数
	ToolCommandManager GetToolCommands(){return m_vecCmd;}

	// ----------------------------------------------------
	// 接口 2: 生成模拟上传数据
	// ----------------------------------------------------
	// buffer: 接收数据的缓冲区
	// nOutLen: 输出总长度
	// vInstList: 仪器数据列表
	void GenerateSimPacket(BYTE* buffer, int& nOutLen, const std::vector<SimInstrumentData>& vInstList);

protected:
	// 虚函数：当解析到具体命令时触发，可在派生类或主逻辑中重写/调用
	virtual void OnCmdStartLog(int nRate);
	virtual void OnCmdStopLog();
	virtual void OnCmdBatchConfig(int nCmdCount, const BYTE* pParamData, int nParamTotalLen, ToolCommandManager& vecCmd);

private:
	// 内部辅助函数
	WORD CalcChecksum(const BYTE* pData, int nLen);
	void ProcessOnePacket(const BYTE* pPacket, int nTotalLen);

private:
	std::vector<BYTE> m_vRecvBuf;	// 接收缓冲区 (处理粘包)
	BYTE m_chRecvFrameCount;		// 发送帧计数（对上位机是接收）
	BYTE m_chErrorFrameCount;		// 出错帧计数

	BOOL m_bStartLogging;			// 开始/结束测井
	BOOL m_bIsRecvParas;			// 接收到有效的参数帧

	// 命令解析
	ToolCommandManager m_vecCmd;	// 命令集合

};

WORD CalcChecksum(WORD* pData, int nWordLen);