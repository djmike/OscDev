//
//  OscTree.cpp
//
//  Created by Michael Latzoni on 8/29/13.
//
//

#include "OscTree.h"
#include <iostream>

using namespace ci;
using namespace std;

OscTree::OscTree()
{
}

OscTree::OscTree( const Buffer& buffer )
{
	// create OscTree from binary data assuming
	// binary data is structed based on the OSC spec
}

/*OscTree::OscTree( const std::string& address )
{
	mAddress	= address;
}*/

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

OscTree::OscTree( float value, uint8_t typeTag )
{
	mValue		= Buffer( sizeof( float ) );
	mValue.copyFrom( &value, sizeof( float ) );
	
	mTypeTag	= typeTag;
}

OscTree::OscTree( const string& value, uint8_t typeTag )
{
	mValue		= Buffer( value.length() + 1 );
	mValue.copyFrom( value.c_str(), mValue.getDataSize() );
	
	mTypeTag	= typeTag;
}

OscTree::OscTree( const char* value, uint8_t typeTag )
{
	mTypeTag	= typeTag;
}

OscTree::OscTree( int64_t value, uint8_t typeTag )
{
	mValue		= Buffer( sizeof( int64_t ) );
	mValue.copyFrom( &value, sizeof( int64_t ) );
	
	mTypeTag	= typeTag;
}

OscTree::OscTree( double value, uint8_t typeTag )
{
	mValue		= Buffer( sizeof( double ) );
	mValue.copyFrom( &value, sizeof( double ) );
	
	mTypeTag	= typeTag;
}

OscTree::OscTree( bool value )
{
    mTypeTag    = ( value ) ? 'T' : 'F';
}

OscTree::OscTree( TimeTag value, uint8_t typeTag )
{
	mTypeTag	= typeTag;
}
 