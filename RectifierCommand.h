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

	typedef struct _StateF05 {
		static const uint8_t hoursIndex = 0x13;
		static const uint8_t minutesIndex = 0x14;
		static const uint8_t secondsIndex = 0x15;
		static const uint8_t lowAByteIndex = 0x16;
		static const uint8_t hiAByteIndex = 0x17;
		static const uint8_t vByteIndex = 0x18;
		static const uint8_t chan1Index = 0xb;
		static const uint8_t chan2Index = 0xc;
		static const uint8_t chan3Index = 0xd;
		static const uint8_t chan4Index = 0xe;
		static const uint8_t controlByteIndex = 0x0a;

		std::vector<uint8_t> stateBytes;

		_StateF05::_StateF05() {
			stateBytes.resize(27);
		}
		

		void setHours(uint8_t value) {
			stateBytes[hoursIndex] = value;
		}
		uint8_t getHours() const {
			return stateBytes[hoursIndex];
		}

		void setMinutes(uint8_t value) {
			stateBytes[minutesIndex] = value;
		}
		uint8_t getMinutes() const {
			return stateBytes[minutesIndex];
		}
		void setSeconds(uint8_t value) {
			stateBytes[secondsIndex] = value;
		}
		uint8_t getSeconds() const {
			return stateBytes[secondsIndex];
		}

		void setLowA(uint8_t lowA) {
			stateBytes[lowAByteIndex] = lowA;
		}
		uint8_t getLowA() const {
			return stateBytes[lowAByteIndex];
		}
		void setHiA(uint8_t hiA) {
			stateBytes[hiAByteIndex] = hiA;
		}
		uint8_t getHiA() const {
			return stateBytes[hiAByteIndex];
		}
		void setV(uint8_t v) {
			stateBytes[lowAByteIndex] = v;
		}
		uint8_t getV() const {
			return stateBytes[vByteIndex];
		}
		void setChan1(uint8_t v) {
			stateBytes[chan1Index] = v;
		}
		uint8_t getChan1() const {
			return stateBytes[chan1Index];
		}
		void setChan2(uint8_t v) {
			stateBytes[chan2Index] = v;
		}
		uint8_t getChan2() const {
			return stateBytes[chan2Index];
		}
		void setChan3(uint8_t v) {
			stateBytes[chan3Index] = v;
		}
		uint8_t getChan3() const {
			return stateBytes[chan3Index];
		}
		void setChan4(uint8_t v) {
			stateBytes[chan4Index] = v;
		}
		uint8_t getChan4() const {
			return stateBytes[chan4Index];
		}
		uint8_t getChan(int chanNumber) const {
			chanNumber = chan1Index + chanNumber - 1;
			return stateBytes[chanNumber];
		}

		uint8_t getControlByte() const {
			return stateBytes[controlByteIndex];
		}

		uint8_t setControlByte(uint8_t controlByte) {
			stateBytes[controlByteIndex] = controlByte;
		}

	} StateF05;

	

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

	static std::vector<uint8_t> createReplyOKFrameSymbols(const uint8_t modbus_addr);

	static std::vector<uint8_t> createReplyFrame(const uint8_t modbus_addr, const uint8_t modbus_func, const uint8_t code, const ReplyStatus status);

	static std::vector<uint8_t> createReplyDataFrame(const uint8_t modbus_addr, const uint8_t modbus_func, const std::vector<uint8_t>& data);

	static std::vector<uint8_t> createRectifierInfoF10(uint8_t lowA, uint8_t hiA, uint8_t lowU, uint8_t hiU, uint8_t configByte, const std::vector<uint8_t>& serialNumber);
	static std::vector<uint8_t> createRectifierStateF07(uint8_t controlByte, uint8_t chan1, uint8_t chan2, uint8_t chan3, uint8_t chan4, uint8_t lowA, uint8_t hiA, uint8_t U);

	static std::vector<uint8_t> createRectifierStateF05(uint8_t hours, uint8_t minutes, uint8_t seconds,
		uint8_t controlByte, uint8_t chan1, uint8_t chan2, uint8_t chan3, uint8_t chan4, uint8_t lowA, uint8_t hiA, uint8_t V);

	static std::vector<uint8_t> createRectifierMemoryData(std::vector<uint8_t> memory, int startAddress, unsigned int numberOfBytes);

	static std::vector<uint8_t> convertToASCIIFrame(const std::vector<uint8_t>& frame_bytes);

	static std::vector<uint8_t> parseASCIIFrameToBytes(const std::vector<uint8_t>& ascii_bytes);

	static std::uint8_t parseCommand(const std::vector<std::uint8_t>& frame_array, uint8_t & modbus_addr, uint8_t & modbus_func, DATA & data);

	static std::uint8_t parseResponseFrame(const std::vector<uint8_t> & bytes,
		uint8_t & modbus_addr,
		uint8_t & modbus_func,
		REPLY_DATA & data
	);

	static std::uint8_t parseResponseCode(const std::vector<std::uint8_t>& frame_array, uint8_t & modbus_addr, uint8_t & modbus_func, uint8_t & replyCode);

	static bool parseDataForF07(const std::vector<std::uint8_t>& rdSymbolsFrame, DeviceCommand::StateF07 & state);
	

	static bool parseDataForF05(const std::vector<std::uint8_t>& data, DeviceCommand::StateF05 & state);
	

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
extern const DeviceCommand::DATA GET_DEVICE_STATE_05;
extern const DeviceCommand::DATA GET_RECTIFIER_STATE_10;
extern const DeviceCommand::DATA ACTIVATE_REMOTE_CONTROL_PANEL_06_01;
extern const DeviceCommand::DATA STOP_EXECUTING_PROGRAMM_06_09;
extern const DeviceCommand::DATA TEST_POWER_MODULES_02_82;
extern const DeviceCommand::DATA SWITCH_OFF_POWER_MODULE_02_80;
