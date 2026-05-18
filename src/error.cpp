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
	case ErrorType::WL_DISPLAY:
		return get_wl_display_error_message(_code);
	case ErrorType::WL_SHM:
		return get_wl_shm_error_message(_code);
	default:
		return "None";
	}
}

Error::operator bool() const
{
	return _type != ErrorType::NONE;
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
		return "Invalid window dimensions";
	case 2:
		return "Invalid color format";
	default:
		return "";
	}
}

std::string Error::get_wl_display_error_message(int code)
{
	switch (code)
	{
	case 0:
		return "Invalid object";
	case 1:
		return "Invalid method";
	case 2:
		return "No memory";
	case 3:
		return "Implementation error";
	default:
		return "";
	}
}

std::string Error::get_wl_shm_error_message(int code)
{
	switch (code)
	{
	case 0:
		return "Invalid Buffer Format";
	case 1:
		return "Invalid stride";
	case 2:
		return "Invalid file descriptor";
	default:
		return "";
	}
}
