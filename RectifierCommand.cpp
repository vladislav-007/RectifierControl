#include "stdafx.h"
#include <vector>
#include "RectifierCommand.h"

const DeviceCommand::DATA GIVE_PREPARED_DATA = { 0x01/*size*/,0x01/*addr*/,0/*cmd*/,std::vector<std::uint8_t>()/*data*/ };
const DeviceCommand::DATA CHECK_CONNECTION_WITH_CPU = { 0x01/*size*/,0x04/*addr*/,0/*cmd*/,std::vector<std::uint8_t>()/*data*/ };
const DeviceCommand::DATA GET_CONCISE_DEVICE_STATE_07 = { 0x01/*size*/,0x07/*addr*/,0/*cmd*/,std::vector<std::uint8_t>()/*data*/ };
const DeviceCommand::DATA GET_RECTIFIER_STATE_10 = { 0x01/*size*/,0x10/*addr*/,0/*cmd*/,std::vector<std::uint8_t>()/*data*/ };

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

std::uint8_t DeviceCommand::bytesToData(const std::vector<uint8_t> & bytes, DATA & cmd_data) {
	// TODO try to use move semantic
	std::uint8_t CRC = 0;

	cmd_data.size = 0;
	if (bytes.empty())
		return CRC;

	
	if (bytes.size() > 0) {
		cmd_data.size = bytes[0];
		CRC += cmd_data.size;
	}
	
	if (bytes.size() > 1) {
		cmd_data.address = bytes[1];
		CRC += cmd_data.address;
	}
		
	if (cmd_data.size > 1 && bytes.size() > 3) {
		cmd_data.command = bytes[2];
		CRC += cmd_data.command;
	}

	for (int8_t i = 3; i < bytes.size()-1; ++i) {
		cmd_data.data.push_back(bytes[i]);
		CRC += bytes[i];
	}
	CRC += bytes[bytes.size() - 1];
	return CRC;
}

std::uint8_t DeviceCommand::parseReplyCode(const std::vector<uint8_t> & bytes, std::uint8_t & replyCode) {
	// TODO try to use move semantic
	std::uint8_t CRC = 0;

	if (bytes.empty())
		return CRC;

	if (bytes.size() > 0) {
		replyCode = bytes[0];
		CRC += replyCode;
	}

	if (bytes.size() > 1) {
		CRC += bytes[1]; // CRC
	}

	return CRC;
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

std::vector<uint8_t> DeviceCommand::createReplyFrame(
	const uint8_t modbus_addr,
	const uint8_t modbus_func,
	const uint8_t code,
	const ReplyStatus status
) {
	std::vector<uint8_t> frame_array;
	uint8_t crc = 0;
	frame_array.push_back(modbus_addr);
	crc += modbus_addr;
	frame_array.push_back(modbus_func);
	crc += modbus_func;

	frame_array.push_back(code);
	crc += code;

	crc = (unsigned char)(0x100 - crc);
	frame_array.push_back(crc);
	return frame_array;
}

std::vector<uint8_t> DeviceCommand::createReplyDataFrame(
	const uint8_t modbus_addr,
	const uint8_t modbus_func,
	const std::vector<uint8_t> & data
) {
	std::vector<uint8_t> frame_array;
	uint8_t crc = 0;
	frame_array.push_back(modbus_addr);
	crc += modbus_addr;
	frame_array.push_back(modbus_func);
	crc += modbus_func;

	for (const auto & byte : data) {
		frame_array.push_back(byte);
		crc += byte;
	}

	crc = (unsigned char)(0x100 - crc);
	frame_array.push_back(crc);
	return frame_array;
}

std::vector<uint8_t> DeviceCommand::createRectifierInfoF10(
	uint8_t lowA, uint8_t hiA,
	uint8_t lowU, uint8_t hiU,
	uint8_t configByte, const std::vector<uint8_t> & serialNumber) {
	std::vector<uint8_t> data(0x10); // from 0x05 byte serial number is placed
	data[0] = lowA; // low byte of current
	data[1] = hiA;	// hi byte of current
	data[2] = lowU; // voltage
	data[3] = hiU;
	data[4] = configByte;
	size_t limitSize = min(0x10 - 0x05, serialNumber.size());
	
	std::copy(serialNumber.begin(), serialNumber.begin() + limitSize, data.begin() + 0x05);
	return data;
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

	//find begin of the frame
	int index = 0;
	for (; index < ascii_bytes.size(); ++index) {
		if (ascii_bytes[index] == ':') {
			++index;
			break;
		}
	}

	// check LRC
	uint8_t check_LRC = 0;
	std::vector<uint8_t> bytes_array;

	for (int i = index; i < ascii_bytes.size()-2; i+=2) {
		if (ascii_bytes[i] == 0x0D && 0x0A == ascii_bytes[i + 1])
			break;
		uint8_t first_simbol = hexToByte(ascii_bytes[i+1]);
		uint8_t second_simbol = hexToByte(ascii_bytes[i]) << 4;
		uint8_t byte = second_simbol + first_simbol;
		bytes_array.push_back(byte);
		check_LRC += byte;
		index = i + 2;
	}

	assert(check_LRC == 0);
	assert(ascii_bytes[index] == 0x0D);
	assert(ascii_bytes[index+1] == 0x0A);
	return bytes_array;
}

std::uint8_t DeviceCommand::parseCommand(const std::vector<std::uint8_t> & frame_array,
	uint8_t & modbus_addr,
	uint8_t & modbus_func,
	DATA & data
) {

	uint8_t crc = 0;
	if (frame_array.size() > 0) {
		modbus_addr = frame_array[0];
		crc += modbus_addr;
	}

	if (frame_array.size() > 1) {
		modbus_func = frame_array[1];
		crc += modbus_func;
	}

	std::vector<std::uint8_t> data_sub_vector(frame_array.cbegin() + 2, frame_array.cend());

	crc += DeviceCommand::bytesToData(data_sub_vector, data);
	assert(0 == crc);
	return crc;
}


std::uint8_t DeviceCommand::parseResponseFrame(const std::vector<std::uint8_t> & frame_array,
	uint8_t & modbus_addr,
	uint8_t & modbus_func,
	DATA & data
) {

	uint8_t crc = 0;
	if (frame_array.size() > 0) {
		modbus_addr = frame_array[0];
		crc += modbus_addr;
	}

	if (frame_array.size() > 1) {
		modbus_func = frame_array[1];
		crc += modbus_func;
	}

	std::vector<std::uint8_t> data_sub_vector(frame_array.cbegin() + 2, frame_array.cend());

	crc +=DeviceCommand::bytesToData(data_sub_vector, data);
	assert(0 == crc);
	return crc;
}

std::uint8_t DeviceCommand::parseResponseCode(const std::vector<std::uint8_t> & frame_array,
	uint8_t & modbus_addr,
	uint8_t & modbus_func,
	uint8_t & replyCode
) {

	uint8_t crc = 0;
	if (frame_array.size() > 0) {
		modbus_addr = frame_array[0];
		crc += modbus_addr;
	}

	if (frame_array.size() > 1) {
		modbus_func = frame_array[1];
		crc += modbus_func;
	}

	std::vector<std::uint8_t> data_sub_vector(frame_array.cbegin() + 2, frame_array.cend());

	crc += DeviceCommand::parseReplyCode(data_sub_vector, replyCode);
	assert(0 == crc);
	return crc;
}

