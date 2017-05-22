#pragma once

namespace io
{
  enum class operation : std::uint8_t { add, del, enable, disable, clear };
  enum class filter : std::uint8_t { read, write, rw, timer };
}
