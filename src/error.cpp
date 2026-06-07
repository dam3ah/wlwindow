#include "error.hpp"
#include <cstring>

Error::Error(ErrorType type, int code) : _type(type), _code(code) {}

ErrorType Error::get_type() const
{
	if (this != nullptr)
		return _type;
	return ErrorType::NONE;
}

int Error::get_code() const
{
	if (this != nullptr)
		return _code;
	return 0;
}

std::string Error::get_message() const
{
	switch (_type)
	{
	case ErrorType::OS:
		return std::strerror(_code);
	case ErrorType::USAGE:
		return get_usage_error_message(_code);
	case ErrorType::LOGIC:
		return get_logic_error_message(_code);
	default:
		return "None";
	}
}

Error::operator bool() const
{
	return _type != ErrorType::NONE;
}

bool Error::is_fatal() const {
	return false;
}

std::string Error::get_logic_error_message(int code)
{
	switch (code)
	{
	case 0:
		return "Buffer ring broken";
	default:
		return "";
	}
}
std::string Error::get_usage_error_message(int code)
{
	switch (code)
	{
	case 0:
		return "Uninitialized state";
	case 1:
		return "Binding Faliure";
	case 2:
		return "Invalid window dimensions";
	case 3:
		return "Invalid color format";
	default:
		return "";
	}
}

FatalError::FatalError(ErrorType type, int code, std::string message) : Error(type, code), message(message) {
}

std::string FatalError::get_message() const{
	return message;
}

bool FatalError::is_fatal() const{
	return true;
}
