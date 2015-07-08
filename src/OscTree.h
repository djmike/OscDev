//
//  OscTree.h
//
//  Created by Michael Latzoni on 8/29/13.
//
//	Based on OSC Spec 1.0
//

#pragma once

#include <chrono>
#include <typeinfo>
#include <string>
#include <vector>
#include "cinder/Buffer.h"
#include "cinder/Exception.h"

class OscTree
{
public:
	typedef uint8_t		TypeTag;

	struct TimeTag
	{
		uint64_t mTimeTag;

		TimeTag() 
			: mTimeTag( 1 )
		{
		}
		
		explicit TimeTag( uint64_t timeTag ) 
			: mTimeTag( timeTag ) 
		{
		}
	};

	//! Creates an empty OscTree
	explicit OscTree();
	
	//! Creates an OscTree from binary data that is structred based on the OSC spec
	explicit OscTree( const ci::BufferRef& buffer );
	
	//! Creates an OscTree that represents an OSC Message
	//explicit OscTree( const std::string& address );
	
	//! Creates an OscTree that represents an OSC Bundle
	//explicit OscTree( const TimeTag& timeTag );

	// OSC Values
	// TODO:
	// implement ci::Color
	
	//! Creates an OscTree that represents an argument that has a custom type tag and no data
	explicit OscTree( TypeTag typeTag );
	
	//! Creates an OscTree that represents a 32-bit integer argument
	explicit OscTree( int32_t value, TypeTag typeTag = 'i' );
	
	//! Creates an OscTree that represents a float argument
	explicit OscTree( float value, TypeTag typeTag = 'f' );
	
	//! Creates an OscTree that represents a string argument
	explicit OscTree( const std::string& value, TypeTag typeTag = 's' );
	
	//! Creates an OscTree that represents a blob argument
	//explicit OscTree( const char* value, size_t numBytes, TypeTag typeTag = 'b' );
	//explicit OscTree( const uint8_t* value, size_t numBytes, TypeTag typeTag = 'b' );
	explicit OscTree( const void* value, size_t numBytes, TypeTag typeTag = 'b' );
	
	//! Creates an OscTree that represents a 64-bit integer argument
	explicit OscTree( int64_t value, TypeTag typeTag = 'h' );
	
	//! Creates an OscTree that represents a double argument
	explicit OscTree( double value, TypeTag typeTag = 'd' );
	
	//! Creates an OscTree that represents a T (true)/F (false) argument
	//explicit OscTree( bool value );
	
	//! Creates an OscTree that represents an OSC-timetag argument
	explicit OscTree( TimeTag timeTag, TypeTag typeTag = 't' );

	//! Creates an OscTree that represents an OSC Message
	static OscTree      makeMessage( const std::string& address );
	
	//! Creates an OscTree that represents an OSC Bundle
	static OscTree      makeBundle( const TimeTag& timeTag = TimeTag() );

	//! Attempt to retrieve the argument value as the requested type
	// this does not work for a string or any object type because
	// the data stored in the buffer will only be the data needed
	// from the object rather than the entire object. For example,
	// in OscTree( string, typetag ), we copy only the character
	// data from the string into the buffer, so when we try to 
	// point to the the data in the buffer as a string*, the data
	// is not a string object, it is character data. We'll have
	// this problem for any larger object type where only the data
	// needed from that object type is copied into the buffer. string
	// and TimeTag definitely will be affected.
	// My solution is to use template specialization
	template <typename T>
	inline T			getValue() const
	{
		return *static_cast<T*>( const_cast<void*>( mValue->getData() ) );
	}
	
	//! Returns the raw binary representation of the value, only valid for an OscTree that represents an argument
	ci::BufferRef		getValue() const { return mValue; }
	
	//! Returns the type tag, only valid for an OscTree that represents argument
	TypeTag				getTypeTag() const { return mTypeTag; }
	
	bool							hasChildren() const;
	std::vector<OscTree>&			getChildren();
	const std::vector<OscTree>&		getChildren() const;
	
	bool				hasParent() const;
	OscTree&			getParent();
	const OscTree&		getParent() const;
	
	//! Appends a child
	void				pushBack( const OscTree& child );
	
	//! Converts entire OscTree structure to binary data based on OSC spec
	ci::BufferRef		toBuffer() const;

	//! Sets the address, applies to OscTrees that represent OSC Messages
	void				setAddress( const std::string& address );

	//! Sets the time tag, applies to OscTress that represent OSC Bundles
	void				setTimeTag( const TimeTag& timeTag );
    
protected:
	std::vector<OscTree>	mChildren;
	OscTree*				mParent;
	ci::BufferRef			mValue;
	std::string				mAddress;
	TimeTag					mTimeTag;
	TypeTag					mTypeTag;
	int32_t					mBlobSize;
	
	void					init();
	void					appendTypeTagString( ci::BufferRef& buffer, size_t& dataSize ) const;
	void					appendAddress( ci::BufferRef& buffer, size_t& dataSize ) const;
	void					appendValue( ci::BufferRef& buffer, size_t& dataSize ) const;
	void					appendValue( ci::BufferRef& buffer, size_t& dataSize, const ci::BufferRef& valueBuffer ) const;
    
public:
	//! Base class for OscTree Exceptions
	class Exception : public ci::Exception
	{
	};

    class ExcExceededMaxSize : public Exception
    {
    public:
        ExcExceededMaxSize( size_t size );
        
        virtual const char* what() const throw()
        {
            return mMessage.c_str();
        }
    protected:
        std::string         mMessage;
    };
};

template<>
inline std::string OscTree::getValue<std::string>() const
{
    return static_cast<const char*>( mValue->getData() );
}

template<>
inline OscTree::TimeTag OscTree::getValue<OscTree::TimeTag>() const
{
    return TimeTag();
}

// TODO:
// throw an exception for trying to read a null
// getValue() == nullptr
// Empty Exception - if data is empty
// Non convertible exception
// dynamic_cast