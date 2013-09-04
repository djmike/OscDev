//
//  OscTree.cpp
//
//  Created by Michael Latzoni on 8/29/13.
//
//

#include "OscTree.h"
#include "cinder/Utilities.h"
#include <limits>

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

/*OscTree::OscTree( const TimeTag& timeTag )
{
	mTimeTag	= timeTag;
}*/

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
	mValue.copyFrom( value.data(), mValue.getDataSize() );
	
	mTypeTag	= typeTag;
}
/*
OscTree::OscTree( const char* value, size_t numBytes, uint8_t typeTag )
{
	mTypeTag	= typeTag;
}

OscTree::OscTree( const uint8_t* value, size_t numBytes, uint8_t typeTag )
{
	mValue		= Buffer( numBytes );
	mValue.copyFrom( value, mValue.getDataSize() );

	mTypeTag	= typeTag;
}
*/
OscTree::OscTree( const void* value, size_t numBytes, uint8_t typeTag )
{
	// according to OSC spec, the data must start with a 32-bit integer
	// size count, followed by that many 8-bit bytes of binary data
	// that means the max value of a 32-bit int is the max size, so
	// check for it
	// should we eliminate this limitation?
	// TODO: create custom exception
	if ( numBytes >= numeric_limits< int32_t >::max() ) {
		throw ExcExceededMaxSize( numBytes );
	}

	mValue		= Buffer( numBytes );
	mValue.copyFrom( value, mValue.getDataSize() );

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

OscTree OscTree::makeMessage( const std::string& address )
{
	OscTree message;
	message.setAddress( address );

	return message;
}

OscTree OscTree::makeBundle( const TimeTag& timeTag )
{
	OscTree bundle;
	bundle.setTimeTag( timeTag );

	return bundle;
}

void OscTree::pushBack( const OscTree& child )
{
	mChildren.push_back( child );
}

Buffer OscTree::toBuffer() const
{
	return Buffer();
}

void OscTree::setAddress( const string& address )
{
	mAddress	= address;
}

void OscTree::setTimeTag( const TimeTag& timeTag )
{
	mTimeTag	= timeTag;
}

OscTree::ExcExceededMaxSize::ExcExceededMaxSize( size_t size )
{
    mMessage    = "Exceeded the maximum size limit. Size: " + toString( size );
}
 