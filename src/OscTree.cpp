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

bool isZeroPadded( const char* p )
{
	const char* q = (const char*)ceil4( (size_t)p );
	for ( ; p < q; ++p ) {
		if ( *p != 0 ) {
			return false;
		}
	}

	return true;
}

OscTree::OscTree()
{
	init();
}

OscTree::OscTree( Buffer buffer )
{
	init();
	// create OscTree from binary data assuming
	// binary data is structed based on the OSC spec
	const char* data = reinterpret_cast<const char*>( buffer.getData() );

	// check if the first byte denotes an OSC Bundle
	// by looking for #bundle at the beginning
	if ( *data == '#' ) {
		// parse OSC Bundle
	} else {
		// parse OSC Message
		// parse out the address pattern
		const char* pBlockEnd	= data + buffer.getDataSize();
		const char* pBegin		= data;
		const char* pEnd		= (const char*)memchr( pBegin, 0, pBlockEnd - pBegin );

		if ( pEnd == nullptr || *pBegin != '/' || *pEnd != '\0' || !isZeroPadded( pEnd + 1 ) ) {
			// the address data is malformed
			// throw an exception?
			// just return an empty OscTree?
		} else {
			setAddress( pBegin );
		}

		// parse the type string
		// TODO:
		// Old OSC implementations are not guaranteed
		// to have a type string. Should we care?
		// For now, I am going to only support
		// OSC implementations that include a
		// type tag string
		pBegin		= (const char*)ceil4( (size_t)pEnd + 1 );
		pEnd		= (const char*)memchr( pBegin, 0, pBlockEnd - pBegin );

		vector<char> typeTags;

		if ( pEnd == nullptr || *pBegin != ',' || !isZeroPadded( pEnd + 1 ) ) {
			// malformed type tag string
		} else {
			// increment pBegin by 1 to exclude comma
			typeTags.assign( pBegin + 1, pEnd );
		}

		// TODO:
		// How do you read data from a buffer
		// if you aren't sure exactly what the
		// data in the buffer is meant to be?
		// We give the user the ability to create
		// custom type tags for all arguments
		// so how can we know what the data
		// in the buffer is?
		// For now, in the spirit of getting this
		// working, I'm going to assume type tags
		// specify argument type, and therefore size
		if ( !typeTags.empty() ) {
			// read arguments
			pBegin	= (const char*)ceil4( (size_t)pEnd + 1 );

			for ( char typeTag : typeTags ) {
				size_t sz = 0;
				if ( pBegin < pBlockEnd ) {
					if ( typeTag == 'i' ) {
						// 32-bit int is 4 bytes
						const int32_t* const pValue	= reinterpret_cast<const int32_t*>( pBegin );
						int32_t value				= *pValue;
						sz							= 4;

						pushBack( OscTree( value, typeTag ) );
					} else if ( typeTag == 'f' ) {
						// float is 4 bytes
						const float* const pValue	= reinterpret_cast<const float*>( pBegin );
						float value					= *pValue;
						sz							= 4;

						pushBack( OscTree( value, typeTag ) );
					} else if ( typeTag == 's' ) {
						pEnd						= reinterpret_cast<const char*>( memchr( pBegin, 0, pBlockEnd - pBegin ) );
						string s					= string( pBegin, pEnd - pBegin );
						sz							= ceil4( s.size() + 1 );

						pushBack( OscTree( s, typeTag ) );
					} else if ( typeTag == 'b' ) {
						// the first 4 bytes of a blob are a 32-bit integer
						// representing the number of 8-bit bytes in the blob
						const int32_t* const pValue	= reinterpret_cast<const int32_t*>( pBegin );
						int32_t blobSize			= *pValue;
						uint8_t* blobData			= new uint8_t[ blobSize ]();
						sz							= ceil4( blobSize + 4 );

						memcpy( blobData, pBegin + 4, blobSize );
						pushBack( OscTree( blobData, blobSize ) );
					} else if ( typeTag == 'h' ) {
						// 64-bit int is 8 bytes
						const int64_t* const pValue	= reinterpret_cast<const int64_t*>( pBegin );
						int64_t value				= *pValue;
						sz							= 8;

						pushBack( OscTree( value, typeTag ) );
					} else if ( typeTag == 'd' ) {
						// double is 8 bytes
						const double* const pValue	= reinterpret_cast<const double*>( pBegin );
						double value				= *pValue;
						sz							= 8;

						pushBack( OscTree( value, typeTag ) );
					} else if ( typeTag == 'T' ) {
						// true value, no bytes allocated,
						// so no need to move pointer
						sz = 0;

						pushBack( OscTree( typeTag ) );
					} else if ( typeTag == 'F' ) {
						// false value, no bytes allocated,
						// so no need to move pointer
						sz = 0;

						pushBack( OscTree( typeTag ) );
					} else if ( typeTag == 'N' ) {
						// nil value, no bytes allocated,
						// so no need to move pointer
						sz = 0;

						pushBack( OscTree( typeTag ) );
					} else if ( typeTag == 'I' ) {
						// infinitum value, no bytes allocated,
						// so no need to move pointer
						sz = 0;

						pushBack( OscTree( typeTag ) );
					} else {
						// unknown type tag
						// should throw error?
					}

					pBegin += sz;
				}
			}
		}
	}
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
	size_t newDataSize			= ( mChildren.size() + 1 + 1 ) * typeTagSize; // need to add 1 for the ',' and 1 for a '\0'
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

	*pBuffer = '\0';
	
	size_t diff			= newDataSizePadded - newDataSize;
	
	if ( diff > 0 ) {
		size_t offset	= oldDataSize + newDataSize;
		padWithZeroes( buffer, offset, diff );
	}
}

void OscTree::appendValue( Buffer &buffer, size_t &dataSize ) const
{
	size_t extraByteSize = 0;
	if ( getTypeTag() == 'b' ) {
		extraByteSize = 4;
	}

	Buffer valueBuffer			= getValue();
	size_t newDataSize			= valueBuffer.getDataSize() + extraByteSize;
	size_t newDataSizePadded	= ceil4( newDataSize );
	size_t oldDataSize			= dataSize;
	size_t newBufferSize		= oldDataSize + newDataSizePadded;
	
	buffer.resize( newBufferSize );
	
	uint8_t* pBuffer	= reinterpret_cast<uint8_t*>( buffer.getData() );
	pBuffer				+= oldDataSize;

	// if this is a blob, a 32-bit int size
	// count needs to be prepended to the data
	if ( getTypeTag() == 'b' ) {
		memcpy( pBuffer, &mBlobSize, extraByteSize );
		pBuffer			+= extraByteSize;
		memcpy( pBuffer, valueBuffer.getData(), mBlobSize );
	} else {
		memcpy( pBuffer, valueBuffer.getData(), newDataSize );
	}
	
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
	