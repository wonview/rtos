/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) 2007 Denys Vlasenko
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */
#include "udhcp/udhcp_common.h"

uint32_t time(void *arg)
{
	return os_getSysTime;
}
