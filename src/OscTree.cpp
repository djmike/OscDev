//
//  OscTree.cpp
//
//  Created by Michael Latzoni on 8/29/13.
//
//

#include "OscTree.h"

using namespace ci;

OscTree::OscTree()
{
}

OscTree::OscTree( const Buffer& buffer )
{
	// create OscTree from binary data assuming
	// binary data is structed based on the OSC spec
}

OscTree::OscTree( const std::string& address )
{
	mAddress	= address;
}

OscTree::OscTree( const TimeTag& timeTag )
{
	mTimeTag	= timeTag;
}

OscTree::OscTree( uint8_t typeTag )
{
	mTypeTag	= typeTag;
}

OscTree::OscTree( int32_t value, uint8_t typeTag )
{
	mValue		= Buffer( sizeof( int32_t ) );
	mValue.copyFrom( &value, sizeof( int32_t ) );
	
	mTypeTag	= typeTag;
}
 