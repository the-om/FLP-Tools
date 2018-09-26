#pragma once

#include <vector>     // vector
#include <cstdlib>    // strtoul
#include <climits>    // ULONG_MAX
#include <stdexcept>  // invalid_argument


class Version {
public:
	Version(char const* ver_str) {
		v.reserve(4);
		char const* p = ver_str;
		for(;;) {
			char* end;
			unsigned long const n = std::strtoul(p, &end, 10);
			if(n != ULONG_MAX) {
				v.push_back(static_cast<unsigned>(n));
				p = end;
				if(*p == '.') {
					p++;
					continue;
				} else if(*p == '\0') {
					break;
				}
			}
			throw std::invalid_argument { "Invalid version string!" };
		}
	}

	bool operator<(Version const& other) {
		std::size_t const min_size = (v.size() < other.v.size() ? v.size() : other.v.size());
		for(std::size_t i = 0; i < min_size; i++) {
			if(v[i] < other.v[i]) {
				return true;
			}
		}
		return false;
	}

	bool operator>=(Version const& other) {
		return !(*this < other);
	}

private:
	std::vector<unsigned> v;
};

