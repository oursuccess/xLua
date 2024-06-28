#include "util/util.h"

namespace CPL
{
	bool CompareIgnoreCase(const std::string& l, const std::string& r)
	{
		if (l.size() != r.size())
			return false;
		for (size_t i = 0; i < l.size(); ++i)
		{
			if (tolower(l[i]) != tolower(r[i]))
				return false;
		}
		return true;
	}
}