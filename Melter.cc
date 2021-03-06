/*
* Melter.cpp
*
*  Created on: Oct 4, 2010
*      Author: daniele
*/

#include <string>
#include <iostream>

#include "Melter.h"
#include "Chunk.h"
#include "IOManager.h"
#include "Mangler.h"

Melter::Melter(::prototype::Parser& parser, ::prototype::Mangler& mangler)
	: _parser(parser), _mangler(mangler), _defective(false)
{
	_io = new IOManager();
	_io->register_source_observer(*this);
}

Melter::~Melter()
{
	delete _io;
}

void Melter::setDefective(bool state)
{
	if(state)
	{
		_io->remove_source_observer(*this);
		_defective = true;
	}
}

bool Melter::isDefective()
{
	return _defective;
}

void Melter::update()
{
	if (!_io)
		assert(false);

	while ( _parser.num_registered_actions() && _io->available( _parser.descriptor() ) ) {
		std::string tag = _parser.tag();

		// get chunk and parse it, check for action expiration
		Chunk chunk = _io->read_for_parsing( _parser.descriptor() );
		bool expired = _parser.call( chunk );

		if(_parser.is_defective())
		{
			if(expired)
				_parser.expire_action();
			_io->append(chunk);

			break;
		}

		_mangler.call(*_io, _parser.tag(), chunk);
		
		if (expired)
			_parser.expire_action();
	}

	_io->process_pending();
}

std::size_t Melter::write( char const * data, std::size_t size )
{
	if (!data)
		return 0;

	
	if(_parser.is_defective() && !_defective)
		setDefective(true);


	Chunk chunk = Chunk( data, BufferDescriptor(0, size) );
	std::size_t ret_size = this->write( chunk );


	return ret_size;
}

std::size_t Melter::write( Chunk &chunk )
{
	if(_defective)
	{
		_io->append(chunk);
		return chunk.size();
	}

	return _io->append_to_input( chunk );
}

bool Melter::empty()
{
	return _io->empty();
}

Chunk Melter::read()
{
	return _io->read_from_output();
}
