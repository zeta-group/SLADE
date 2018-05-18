#pragma once

#include "Main.h"

namespace CIE
{
double CIE76(ColLAB& col1, ColLAB& col2);
double CIE94(ColLAB& col1, ColLAB& col2);
double CIEDE2000(ColLAB& col1, ColLAB& col2);
} // namespace CIE
