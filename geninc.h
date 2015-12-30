#ifndef __geninc_h__
#define __geninc_h__

#include "dbdiff.h"
#include <iostream>
#include <vector>

void GenerateHeaderFile(std::vector<dbdiff::SchemaPtr>& vec, std::ostream& os);

#endif // __geninc_h__
