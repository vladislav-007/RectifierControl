#pragma once
#include <map>
#include <vector>



class DeviceCommand
{
public:
	typedef struct _DATA {
		uint8_t size; /*bytes number in the command*/
		uint8_t address; /*adress of device or function code*/
		uint8_t command;
		std::vector<uint8_t> data;
	} DATA, *LPDATA;

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
	static std::uint8_t DeviceCommand::bytesToData(const std::vector<uint8_t> & bytes, DATA & data);

	static std::vector<uint8_t> createCmdFrame(
		const uint8_t modbus_addr,
		const uint8_t modbus_func,
		const DATA data
	);

	static std::vector<uint8_t> convertToASCIIFrame(const std::vector<uint8_t>& frame_bytes);

	static std::vector<uint8_t> parseASCIIFrameToBytes(const std::vector<uint8_t>& ascii_bytes);

	static std::uint8_t parseResponseFrame(const std::vector<uint8_t> & bytes,
		uint8_t & modbus_addr,
		uint8_t & modbus_func,
		DATA & data
	);




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