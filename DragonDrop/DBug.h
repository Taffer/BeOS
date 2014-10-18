#ifndef DBug_H
#define DBug_H

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <null.h>

class DBug
{
public:
	DBug( const char *str, bool once = false )
		: _str( NULL ),
		  _one_message( once )
	{
		_str = strdup( str );

		if( !_one_message ) {
			fprintf( stderr, ">>> entering %s\n", _str );
		} else {
			fprintf( stderr, ">>> %s\n", _str );
		}
	}

	~DBug( void )
	{
		if( !_one_message ) fprintf( stderr, "<<< exitting %s\n", _str );

		free( const_cast<char *>( _str ) );
	}

private:
	const char *_str;
	bool _one_message;
};

#endif
