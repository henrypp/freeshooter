// Free Shooter
// Copyright (c) 2009-2023 Henry++

#pragma once

#include "routine.h"

#include <mmsystem.h>

#include "app.h"
#include "rapp.h"
#include "main.h"

#include "resource.h"

DECLSPEC_SELECTANY STATIC_DATA config = {0};

DECLSPEC_SELECTANY R_FREE_LIST context_list = {0};

DECLSPEC_SELECTANY const LONG timer_array[] =
{
	1,
	2,
	3,
	5,
	7,
	9,
};

#include "dialog.h"
#include "helper.h"
#include "image.h"
