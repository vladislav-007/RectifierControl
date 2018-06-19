#include "stdafx.h"
#include <vector>
#include "RectifierCommand.h"

const DeviceCommand::DATA GIVE_PREPARED_DATA = { 0x01/*size*/,0x01/*addr*/,0/*cmd*/,0/*data*/ };
const DeviceCommand::DATA CHECK_CONNECTION_WITH_CPU = { 0x01/*size*/,0x04/*addr*/,0/*cmd*/,0/*data*/ };
const DeviceCommand::DATA GET_CONCISE_DEVICE_STATE_07 = { 0x01/*size*/,0x07/*addr*/,0/*cmd*/,0/*data*/ };
const DeviceCommand::DATA GET_RECTIFIER_STATE_10 = { 0x01/*size*/,0x10/*addr*/,0/*cmd*/,0/*data*/ };

DeviceCommand::DeviceCommand()
{
}

DeviceCommand::DeviceCommand(const DeviceCommand & src)
{
	this->cmd_data_ = src.cmd_data_;
}

DeviceCommand DeviceCommand::operator=(const DeviceCommand &src)
{
	if (this != &src) {
		this->cmd_data_ = src.cmd_data_;
	}
	return *this;
}

DeviceCommand::~DeviceCommand()
{
}

std::vector<uint8_t> DeviceCommand::dataToVector(DATA cmd_data) {
	std::vector<uint8_t> data_array;
	data_array.push_back(cmd_data.size);
	if (cmd_data.size > 0)
		data_array.push_back(cmd_data.address);
	if (cmd_data.size > 1)
		data_array.push_back(cmd_data.command);
	for (int8_t i = 0; i < cmd_data.size - 2; ++i) {
		data_array.push_back(cmd_data.data[i]);
	}
	return data_array;
}

std::vector<uint8_t> DeviceCommand::createCmdFrame(
	const uint8_t modbus_addr,
	const uint8_t modbus_func,
	const DATA data
) {
	std::vector<uint8_t> frame_array;
	uint8_t crc = 0;
	frame_array.push_back(modbus_addr);
	crc += modbus_addr;
	frame_array.push_back(modbus_func);
	crc += modbus_func;
	for (auto byte : dataToVector(data)) {
		frame_array.push_back(byte);
		crc += byte;
	}
	crc = (unsigned char)(0x100 - crc);
	frame_array.push_back(crc);
	return frame_array;
}

std::vector<uint8_t> DeviceCommand::convertToASCIIFrame(const std::vector<uint8_t> & frame_bytes) {
	std::vector<uint8_t> ascii_array;
	ascii_array.push_back(':'); // ASCII start
								// every byte in ascii is two byte e.g. 0x23 -> 0x32,0x33
	const char hexSymbols[] = {'0', '1','2', '3','4', '5','6', '7','8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	uint8_t check_LRC = 0;
	for (const auto & byte : frame_bytes) {
		check_LRC += byte;
		uint8_t first_simbol = hexSymbols[(0x0F & byte)];
		uint8_t second_simbol = hexSymbols[(0xF0 & byte) >> 4];
		ascii_array.push_back(second_simbol);
		ascii_array.push_back(first_simbol);
	}
	//assert(check_LRC == 0);
	ascii_array.push_back(0x0D);
	ascii_array.push_back(0x0A);
	return ascii_array;
}

