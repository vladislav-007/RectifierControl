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

DeviceCommand::DATA DeviceCommand::bytesToData(const std::vector<uint8_t> & bytes) {
	// TODO try to use move semantic
	DATA cmd_data;
	cmd_data.size = 0;
	if (bytes.empty())
		return cmd_data;

	cmd_data.size = 1 + bytes.size();
	
	if (bytes.size() > 0)
		cmd_data.address = bytes[0];
		
	if (bytes.size() > 1)
		cmd_data.command = bytes[1];

	for (int8_t i = 2; i < bytes.size(); ++i) {
		cmd_data.data[i] = bytes[i];
	}

	return cmd_data;
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

uint8_t hexToByte(uint8_t asciiSymbol) {
	switch (asciiSymbol) {
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	case 'A': return 10;
	case 'a': return 10;
	case 'B': return 11;
	case 'b': return 11;
	case 'C': return 12;
	case 'c': return 12;
	case 'D': return 13;
	case 'd': return 13;
	case 'E': return 14;
	case 'e': return 14;
	case 'F': return 15;
	case 'f': return 15;
	default:
		throw std::exception("Not hex symbol");
	}
}

std::vector<uint8_t> DeviceCommand::parseASCIIFrameToBytes(const std::vector<uint8_t> & ascii_bytes) {

	// check LRC
	uint8_t check_LRC = 0;
	std::vector<uint8_t> bytes_array;
	assert(ascii_bytes[0] == ':');
	check_LRC = ascii_bytes[0];
	bytes_array.push_back(ascii_bytes[0]);
	for (int i = 0; i < ascii_bytes.size()-2; i+=2) {
	
		uint8_t first_simbol = hexToByte(ascii_bytes[i+1]);
		uint8_t second_simbol = hexToByte(ascii_bytes[i]) << 4;
		uint8_t byte = second_simbol + first_simbol;
		bytes_array.push_back(byte);
		check_LRC += byte;
	}

	assert(check_LRC == 0);
	bytes_array.push_back(0x0D);
	bytes_array.push_back(0x0A);
	return bytes_array;
}


std::vector<uint8_t> DeviceCommand::parseCmdFrame(const std::vector<std::uint8_t> & frame_array,
	uint8_t & modbus_addr,
	uint8_t & modbus_func,
	DATA & data
) {
	//std::vector<uint8_t> frame_array;
	uint8_t crc = 0;
	modbus_addr = frame_array[0];
	crc += modbus_addr;
	modbus_func = frame_array[1];
	crc += modbus_func;
	for (auto byte : dataToVector(data)) {
		frame_array.push_back(byte);
		crc += byte;
	}
	crc = (unsigned char)(0x100 - crc);
	frame_array.push_back(crc);
	return frame_array;
}

