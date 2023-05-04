#include <iostream>
#include <quantisation.h>

#include <cstdint>
#include <sstream>
#include <string>

int main()
{
    {
        std::array lo{0.f, -10.f, 0.f};
        std::array hi{1.f, 10.f, 10.f};

        PackedVec3<std::uint64_t, 20, 5, 30>  v({0.7f, 5.0f, 2.f}, lo, hi);

        std::cout << v.packedVal << '\n';
        auto a = v.unpack(lo, hi);
        std::cout << a[0] << ' ' << a[1] << ' ' << a[2] << '\n';
    }

    {
        std::string s;
        std::ostringstream os(s);

        std::uint32_t v = 0;
        while (std::cin >> v)
        {
            write_packed_uint32(v, os);
        }

        std::cout << "size: " << os.str().size() << '\n';

        std::istringstream is(os.str());
        while (is)
        {
            std::cout << read_packed_uint32(is) << '\n';
        }
    }

    return 0;
}

