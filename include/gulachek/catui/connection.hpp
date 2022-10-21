#ifndef GULACHEK_CATUI_CONNECTION_HPP
#define GULACHEK_CATUI_CONNECTION_HPP

#include <gulachek/gtree.hpp>

namespace gulachek::catui
{
	class CATUI_API connection
	{
		public:
			connection(
					int in = -1,
					int out = -1,
					bool is_remote = false
					) :
				in_{in},
				out_{out},
				is_remote_{is_remote}
			{}

			~connection();

			int in() const;
			void in(int val);

			int out() const;
			void out(int val);

			// shouldn't branch on this but would rather
			// expose than have people test fd attributes
			bool is_remote() const;
			void is_remote(bool val);

			error close();

			template <gtree::encodable E>
			error write(const E &val)
			{ return gtree::write_fd(out_, val); }

			template <gtree::decodable D>
			error read(D *val)
			{ return gtree::read_fd(in_, val); }

		private:
			int in_;
			int out_;
			bool is_remote_;
	};
}

#endif
