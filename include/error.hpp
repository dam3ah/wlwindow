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
	UNKNOWN,
	WL_SHM,
	XDG_SHELL,
};

struct Error
{
public:
	Error(ErrorType type, int code);
	ErrorType get_type() const;
	int get_code() const;
	virtual std::string get_message() const;
	virtual bool is_fatal() const;
	explicit operator bool() const;

protected:
	int _code;
	ErrorType _type;
	Error &operator=(const Error &other) = delete;
private:
	static std::string get_usage_error_message(int code);
	static std::string get_logic_error_message(int code);
};

struct FatalError : public Error {
public:
	virtual std::string get_message() const override;
	virtual bool is_fatal() const override;
	FatalError(ErrorType type, int code, std::string message);
private:
	std::string message;
};
