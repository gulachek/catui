#include "gulachek/catui/connection.hpp"

#include <unistd.h>

namespace gulachek::catui
{
	connection::~connection()
	{
		close();
	}

	int connection::in() const
	{ return in_; }

	void connection::in(int val)
	{ in_ = val; }

	int connection::out() const
	{ return out_; }

	void connection::out(int val)
	{ out_ = val; }

	bool connection::is_remote() const
	{ return is_remote_; }

	void connection::is_remote(bool val)
	{ is_remote_ = val; }

	error connection::close()
	{
		int in = in_, out = out_;
		in_ = out_ = -1;

		if (::close(in))
		{
			error err;
			err << "failed to close in stream " << in << ": " << ::strerror(errno);
			return err;
		}

		if (out != in && ::close(out))
		{
			error err;
			err << "failed to close out stream " << out << ": " << ::strerror(errno);
			return err;
		}

		return {};
	}
}
