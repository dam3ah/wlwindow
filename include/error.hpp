#pragma once
#include <cerrno>
#include <string>

enum class ErrorType
{
	NONE,
	USAGE,
	LOGIC,
	OS,
	WL_DISPLAY,
	WL_SHM,
};

struct Error
{
public:
	Error(ErrorType type, int code);
	ErrorType get_type() const;
	int get_code() const;
	std::string get_message() const;
	explicit operator bool() const;

private:
	int _code;
	ErrorType _type;
	static std::string get_wl_display_error_message(int code);
	static std::string get_wl_shm_error_message(int code);
	static std::string get_usage_error_message(int code);
	static std::string get_logic_error_message(int code);
	Error &operator=(const Error &other) = delete;
};
