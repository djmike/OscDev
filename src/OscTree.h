//
//  OscTree.h
//
//  Created by Michael Latzoni on 8/29/13.
//
//

#pragma once

#include <typeinfo>
#include <string>
#include <vector>
#include "cinder/Buffer.h"
#include "boost/date_time/posix_time/posix_time.hpp"

class OscTree
{
public:
	typedef boost::posix_time::ptime	TimeTag;

	//! Creates an empty OscTree
	explicit OscTree();
	
	//! Creates an OscTree from binary data that is structred based on the OSC spec
	explicit OscTree( const ci::Buffer& buffer );
	
	//! Creates an OscTree that represents an OSC Message
	explicit OscTree( const std::string& address );
	
	//! Creates an OscTree that represents an OSC Bundle
	explicit OscTree( const TimeTag& timeTag = boost::posix_time::microsec_clock::local_time() );

	// OSC Values
	// TODO:
	// implement ci::Color
	
	//! Creates an OscTree that represents a no-data, custom type tag argument
	explicit OscTree( uint8_t typeTag );
	
	//! Creates an OscTree that represents a 32-bit integer argument
	explicit OscTree( int32_t value, uint8_t typeTag = 'i' );
	
	//! Creates an OscTree that represents a float argument
	explicit OscTree( float value, uint8_t typeTag = 'f' );
	
	//! Creates an OscTree that represents a string argument
	explicit OscTree( const std::string& value, uint8_t typeTag = 's' );
	
	//! Creates an OscTree that represents a blob argument
	explicit OscTree( const char* value, uint8_t typeTag = 'b' );
	
	//! Creates an OscTree that represents a 64-bit integer argument
	explicit OscTree( int64_t value, uint8_t typeTag = 'h' );
	
	//! Creates an OscTree that represents a double argument
	explicit OscTree( double value, uint8_t typeTag = 'd' );
	
	//! Creates an OscTree that represents a T (true)/F (false) argument
	explicit OscTree( bool value );
	
	//! Creates an OscTree that represents an OSC-timetag argument
	explicit OscTree( uint64_t timeTag, uint8_t typeTag = 't' );

	//! Creates an OscTree that represents an OSC Message
	static OscTree      makeMessage( const std::string& address );
	
	//! Creates an OscTree that represents an OSC Bundle
	static OscTree      makeBundle( const TimeTag& timeTag = boost::posix_time::microsec_clock::local_time() );

	//! Attempt to retrieve the argument value as the requested type
	template <typename T>
	inline T			getValue() const
	{
		return *reinterpret_cast<T*>( const_cast<void*>( mValue.getData() ) );
	}
	
	//! Returns the raw binary representation of the value
	ci::Buffer			getValue() const { return mValue; }
	
	//! Retruns the type tag
	uint8_t				getTypeTag() const { return mTypeTag; }
	
	//! Appends a child
	void				pushBack( const OscTree& child );
	
	//! Converts entire OscTree structure to binary data based on OSC spec
	ci::Buffer          toBuffer() const;
    
protected:
	std::vector<OscTree>    mChildren;
	ci::Buffer              mValue;
	std::string             mAddress;
	TimeTag                 mTimeTag;
	uint8_t                 mTypeTag;
};

// throw an exception for trying to read a null
// getValue() == nullptr
// Empty Exception - if data is empty
// Non convertible exception
// dynamic_cast