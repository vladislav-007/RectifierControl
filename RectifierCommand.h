#pragma once
#include <map>
#include <vector>

enum class ReplyStatus
{
	OK,
	Error
};


class DeviceCommand
{
public:
	typedef struct _DATA {
		uint8_t size; /*bytes number in the command*/
		uint8_t address; /*adress of device or function code*/
		uint8_t command;
		std::vector<uint8_t> data;

	} DATA, *LPDATA;

	typedef struct _REPLY_DATA {
		std::vector<uint8_t> data;
	} REPLY_DATA, *LPREPLY_DATA;

	typedef struct _StateF07 {
		_StateF07::_StateF07() {
			control = 0;
			aLow = -1; // current
			aHi = -1; // current
			V = -1;
		}

		uint8_t control;
		uint8_t channel_state[4];
		uint8_t aLow; // current
		uint8_t aHi; // current
		uint8_t V;
	} StateF07;

	

	static const DeviceCommand::DATA GIVE_PREPARED_DATA_01;

	static DeviceCommand& getSimpleCommand(const std::uint8_t cmdNumber) {

		if (simpleCmds_.find(cmdNumber) == simpleCmds_.end()) {
			DATA simple_data = { 0x01/*size*/,0x07/*addr*/,0/*cmd*/, std::vector<std::uint8_t>()/*data*/ };
			simple_data.command = cmdNumber;

			DeviceCommand cmd(simple_data);
			//std::map<uint8_t, DeviceCommand>::value_type val((const std::uint8_t&)cmdNumber,(const DeviceCommand&) cmd);
			simpleCmds_[cmdNumber] = cmd;
		}

		return simpleCmds_[cmdNumber];

	}

	static std::vector<uint8_t> dataToVector(DATA cmd_data);
	static std::uint8_t DeviceCommand::commandBytesToData(const std::vector<uint8_t> & bytes, DATA & data);
	static std::uint8_t DeviceCommand::replyBytesToData(const std::vector<uint8_t> & bytes, DATA & data);

	static std::uint8_t parseReplyCode(const std::vector<uint8_t>& bytes, std::uint8_t & replayCode);

	static std::vector<uint8_t> createCmdFrame(
		const uint8_t modbus_addr,
		const uint8_t modbus_func,
		const DATA data
	);

	static std::vector<uint8_t> createReplyFrame(const uint8_t modbus_addr, const uint8_t modbus_func, const uint8_t code, const ReplyStatus status);

	static std::vector<uint8_t> createReplyDataFrame(const uint8_t modbus_addr, const uint8_t modbus_func, const std::vector<uint8_t>& data);

	static std::vector<uint8_t> createRectifierInfoF10(uint8_t lowA, uint8_t hiA, uint8_t lowU, uint8_t hiU, uint8_t configByte, const std::vector<uint8_t>& serialNumber);
	static std::vector<uint8_t> createRectifierStateF07(uint8_t controlByte, uint8_t chan1, uint8_t chan2, uint8_t chan3, uint8_t chan4, uint8_t lowA, uint8_t hiA, uint8_t U);

	static std::vector<uint8_t> convertToASCIIFrame(const std::vector<uint8_t>& frame_bytes);

	static std::vector<uint8_t> parseASCIIFrameToBytes(const std::vector<uint8_t>& ascii_bytes);

	static std::uint8_t parseCommand(const std::vector<std::uint8_t>& frame_array, uint8_t & modbus_addr, uint8_t & modbus_func, DATA & data);

	static std::uint8_t parseResponseFrame(const std::vector<uint8_t> & bytes,
		uint8_t & modbus_addr,
		uint8_t & modbus_func,
		REPLY_DATA & data
	);

	static std::uint8_t parseResponseCode(const std::vector<std::uint8_t>& frame_array, uint8_t & modbus_addr, uint8_t & modbus_func, uint8_t & replyCode);

	static StateF07 parseDataForF07(const std::vector<std::uint8_t>& rdSymbolsFrame);
	

	DeviceCommand(const DATA & cmd_data) {
		this->cmd_data_ = cmd_data;
	};

	DeviceCommand();

private:
	DATA cmd_data_;

private:
	DeviceCommand(const DeviceCommand&);
	DeviceCommand operator=(const DeviceCommand&);
	static std::map<uint8_t, DeviceCommand> simpleCmds_;
	static std::map<std::pair<uint8_t, uint8_t>, DeviceCommand> cmds_;

public:
	virtual ~DeviceCommand();
};

extern const DeviceCommand::DATA GET_CONCISE_DEVICE_STATE_07;
extern const DeviceCommand::DATA GET_RECTIFIER_STATE_10;