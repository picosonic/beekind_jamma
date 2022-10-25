//=============================================================================
//	FILE:					metadata.cpp
//	SYSTEM:				
//	DESCRIPTION:
//-----------------------------------------------------------------------------
//  COPYRIGHT:		(C)Copyright 2022 Jasper Renow-Clarke. All Rights Reserved.
//	LICENCE:			MIT
//=============================================================================
#include <cstdint>
#include "engine/engine.h"

static const jammagame::MetaData sg_metadata =
{
	"BeeKind",
	"Bee Kind platformer",
	{ "beekind", nullptr },
	1,0,
	JG_API_MAJOR, 
	JG_API_MINOR
};

const jammagame::MetaData * 	
jammagame_get_metadata()
{
	return &sg_metadata;
}

