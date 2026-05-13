#include "HashEngine.h"

BCRYPT_ALG_HANDLE g_bcrypt_alg_ = nullptr;
std::once_flag s_init_flag;
