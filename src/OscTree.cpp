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

size_t ceil4( size_t size )
{
	size_t remainder = size % 4;
	if ( remainder == 0 ) {
		return size;
	}
	
	return size + 4 - remainder;
}

void padWithZeroes( Buffer& buffer, size_t offset, size_t length )
{
	uint8_t* data	= reinterpret_cast<uint8_t*>( buffer.getData() );
	data			+= offset;
	
	memset( data, 0, length );
}

void padWithZeroes( Buffer& buffer )
{
	size_t oldSize		= buffer.getDataSize();
	size_t newSize		= ceil4( oldSize );
	size_t diff			= newSize - oldSize;
	
	if ( diff > 0 ) {
		buffer.resize( newSize );
		padWithZeroes( buffer, oldSize, diff );
	}
}

OscTree::OscTree()
{
	init();
}

OscTree::OscTree( const Buffer& buffer )
{
	init();
	// create OscTree from binary data assuming
	// binary data is structed based on the OSC spec
	
	// check if the first byte denotes an OSC Bundle
	// by looking for #bundle
	
	// if it is not a bundle, it is an OSC Message
	
}

/*OscTree::OscTree( const std::string& address )
{
	mAddress	= address;
}*/

/*OscTree::OscTree( const TimeTag& timeTag )
{
	mTimeTag	= timeTag;
}*/

OscTree::OscTree( TypeTag typeTag )
{
	init();
	
	mTypeTag	= typeTag;
}

OscTree::OscTree( int32_t value, TypeTag typeTag )
{
	init();
	
	mValue		= Buffer( sizeof( int32_t ) );
	mValue.copyFrom( &value, sizeof( int32_t ) );
	
	mTypeTag	= typeTag;
}

OscTree::OscTree( float value, TypeTag typeTag )
{
	init();
	
	mValue		= Buffer( sizeof( float ) );
	mValue.copyFrom( &value, sizeof( float ) );
	
	mTypeTag	= typeTag;
}

OscTree::OscTree( const string& value, TypeTag typeTag )
{
	init();
	
	mValue		= Buffer( value.length() + 1 );
	mValue.copyFrom( value.data(), mValue.getDataSize() );
	
	mTypeTag	= typeTag;
}
/*
OscTree::OscTree( const char* value, size_t numBytes, TypeTag typeTag )
{
	mTypeTag	= typeTag;
}

OscTree::OscTree( const uint8_t* value, size_t numBytes, TypeTag typeTag )
{
	mValue		= Buffer( numBytes );
	mValue.copyFrom( value, mValue.getDataSize() );

	mTypeTag	= typeTag;
}
*/
OscTree::OscTree( const void* value, size_t numBytes, TypeTag typeTag )
{
	// according to OSC spec, the data must start with a 32-bit integer
	// size count, followed by that many 8-bit bytes of binary data
	// that means the max value of a 32-bit int is the max size, so
	// check for it
	// should we eliminate this limitation?
	if ( numBytes >= numeric_limits< int32_t >::max() ) {
		throw ExcExceededMaxSize( numBytes );
	}
	
	init();

	mValue		= Buffer( numBytes );
	mValue.copyFrom( value, mValue.getDataSize() );

	mTypeTag	= typeTag;
	mBlobSize	= numBytes;
}

OscTree::OscTree( int64_t value, TypeTag typeTag )
{
	init();
	
	mValue		= Buffer( sizeof( int64_t ) );
	mValue.copyFrom( &value, sizeof( int64_t ) );
	
	mTypeTag	= typeTag;
}

OscTree::OscTree( double value, TypeTag typeTag )
{
	init();
	
	mValue		= Buffer( sizeof( double ) );
	mValue.copyFrom( &value, sizeof( double ) );
	
	mTypeTag	= typeTag;
}

//OscTree::OscTree( bool value )
//{
//	init();
//	
//    mTypeTag    = ( value ) ? 'T' : 'F';
//}

OscTree::OscTree( TimeTag value, TypeTag typeTag )
{
	init();
	
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

bool OscTree::hasChildren() const
{
	return !mChildren.empty();
}

vector<OscTree>& OscTree::getChildren()
{
	return mChildren;
}

const vector<OscTree>& OscTree::getChildren() const
{
	return mChildren;
}

bool OscTree::hasParent() const
{
	return mParent != nullptr;
}

void OscTree::pushBack( const OscTree& child )
{
	mChildren.push_back( child );
	mChildren.back().mParent = this;
}

Buffer OscTree::toBuffer() const
{
	// Turn the entire OscTree data structure into binary data
	// if this contains children, it is either an OSC Message or an OSC Bundle
	// if this's children contain children, it is an OSC Bundle

	// if this is an OSC Bundle, write #bundle as the first byte in the buffer
	// followed by an OSC time tag
	// then write each bundle element into the buffer by specifying the
	// element size in 8-bit bytes followed by the element data
	
	Buffer buffer( 4 );
	size_t dataSize = 0;
	
	// if this node doesn't have children, it represents
	// an argument. return the binary representation of
	// the argument, and pad it so it is a multiple of 4
	if ( !hasChildren() ) {
		appendValue( buffer, dataSize );
	} else {
		bool isBundle = false;
		
		for ( const auto& child : mChildren ) {
			if ( child.hasChildren() ) {
				isBundle	= true;
				break;
			}
		}
		
		if ( isBundle ) {
			
		} else {
			// write address pattern into buffer
			appendAddress( buffer, dataSize );
			
			// write type tag string into buffer
			appendTypeTagString( buffer, dataSize );
		}
		
		for ( const auto& child : mChildren ) {
			if ( child.hasChildren() ) {
				// this is a bundle
				// child is a bundle or a message
			} else {
				// an OSC Message contains an OSC Address Pattern
				// followed by an OSC Type String
				// followed by zero or more OSC Arguments
				
				// this OscTree is a message
				// each child is an argument
				// and these come back zero padded
//				Buffer childBuffer		= child.toBuffer();
				appendValue( buffer, dataSize, child.toBuffer() );
			}
		}
	}
	
	return buffer;
}

void OscTree::appendAddress( Buffer& buffer, size_t& dataSize ) const
{
	size_t newDataSize			= mAddress.size() + 1; // add 1 for null terminator not included in string::size()
	size_t newDataSizePadded	= ceil4( newDataSize );
	size_t oldDataSize			= dataSize;
	size_t newBufferDataSize	= oldDataSize + newDataSizePadded;
	dataSize					= newBufferDataSize;
	
	buffer.resize( newBufferDataSize );
	
	uint8_t* pBuffer			= reinterpret_cast<uint8_t*>( buffer.getData() );
	pBuffer						+= oldDataSize;
	
	memcpy( pBuffer, mAddress.data(), newDataSize );
	
	size_t diff					= newDataSizePadded - newDataSize;
	
	if ( diff > 0 ) {
		size_t offset	= oldDataSize + newDataSize;
		padWithZeroes( buffer, offset, diff );
	}
}

void OscTree::appendTypeTagString( Buffer& buffer, size_t& dataSize ) const
{
	size_t typeTagSize			= sizeof( TypeTag );
	size_t newDataSize			= ( mChildren.size() + 1 ) * typeTagSize; // need to add 1 for the ','
	size_t newDataSizePadded	= ceil4( newDataSize );
	size_t oldDataSize			= dataSize;
	size_t newBufferDataSize	= oldDataSize + newDataSizePadded;
	dataSize					= newBufferDataSize;
	
	buffer.resize( newBufferDataSize );
	
	uint8_t* pBuffer	= reinterpret_cast<uint8_t*>( buffer.getData() );
	pBuffer				+= oldDataSize;
	
	memcpy( pBuffer, ",", typeTagSize );
	pBuffer				+= typeTagSize;
	
	for ( const auto& child : mChildren ) {
		TypeTag typeTag		= child.getTypeTag();
		memcpy( pBuffer, &typeTag, typeTagSize );
		pBuffer				+= typeTagSize;
	}
	
	size_t diff			= newDataSizePadded - newDataSize;
	
	if ( diff > 0 ) {
		size_t offset	= oldDataSize + newDataSize;
		padWithZeroes( buffer, offset, diff );
	}
}

void OscTree::appendValue( Buffer &buffer, size_t &dataSize ) const
{
	Buffer valueBuffer			= getValue();
	size_t newDataSize			= valueBuffer.getDataSize();
	size_t newDataSizePadded	= ceil4( newDataSize );
	size_t oldDataSize			= dataSize;
	size_t newBufferSize		= oldDataSize + newDataSizePadded;
	
	buffer.resize( newBufferSize );
	
	uint8_t* pBuffer	= reinterpret_cast<uint8_t*>( buffer.getData() );
	pBuffer				+= oldDataSize;
	
	memcpy( pBuffer, valueBuffer.getData(), newDataSize );
	
	size_t diff			= newDataSizePadded - newDataSize;
	
	if ( diff > 0 ) {
		size_t offset	= oldDataSize + newDataSize;
		padWithZeroes( buffer, offset, diff );
	}
}

// Appends the data from a buffer holding argument/value data to a buffer
// The value data should already be zero padded
void OscTree::appendValue( ci::Buffer& buffer, size_t& dataSize, const ci::Buffer& valueBuffer ) const
{
	size_t newDataSize		= valueBuffer.getDataSize();
	size_t oldDataSize		= dataSize;
	size_t newBufferSize	= dataSize + newDataSize;
	dataSize				= newBufferSize;
	
	buffer.resize( newBufferSize );
	
	uint8_t* pBuffer		= reinterpret_cast<uint8_t*>( buffer.getData() );
	pBuffer					+= oldDataSize;
	
	memcpy( pBuffer, valueBuffer.getData(), newDataSize );
}

void OscTree::setAddress( const string& address )
{
	mAddress	= address;
}

void OscTree::setTimeTag( const TimeTag& timeTag )
{
	mTimeTag	= timeTag;
}

void OscTree::init()
{
	mParent		= nullptr;
	mTypeTag	= 0;
	mBlobSize	= 0;
}

OscTree::ExcExceededMaxSize::ExcExceededMaxSize( size_t size )
{
    mMessage    = "Exceeded the maximum size limit. Size: " + toString( size );
}
	