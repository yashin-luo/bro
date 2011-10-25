// See the file "COPYING" in the main distribution directory for copyright.

#include "InputReaderAscii.h"
#include "DebugLogger.h"

#include <sstream>

FieldMapping::FieldMapping(const string& arg_name, const TypeTag& arg_type, int arg_position) 
	: name(arg_name), type(arg_type)
{
	position = arg_position;
}

FieldMapping::FieldMapping(const FieldMapping& arg) 
	: name(arg.name), type(arg.type)
{
	position = arg.position;
}

InputReaderAscii::InputReaderAscii()
{
	//DBG_LOG(DBG_LOGGING, "input reader initialized");
	file = 0;
}

InputReaderAscii::~InputReaderAscii()
{
}

void InputReaderAscii::DoFinish()
{
}

bool InputReaderAscii::DoInit(string path, int num_fields, const LogField* const * fields)
{
	fname = path;
	
	file = new ifstream(path.c_str());
	if ( !file->is_open() ) {
		Error(Fmt("cannot open %s", path.c_str()));
		return false;
	}

	// try to read the header line...
	string line;
	if ( !getline(*file, line) ) {
		Error("could not read first line");
		return false;
	}
	 
	// split on tabs...
	istringstream splitstream(line);
	unsigned int currTab = 0;
	int wantFields = 0;
	while ( splitstream ) {
		string s;
		if ( !getline(splitstream, s, '\t'))
			break;
		
		// current found heading in s... compare if we want it
		for ( int i = 0; i < num_fields; i++ ) {
			const LogField* field = fields[i];
			if ( field->name == s ) {
				// cool, found field. note position
				FieldMapping f(field->name, field->type, i);
				columnMap.push_back(f);
				wantFields++;
				break; // done with searching
			}
		}

		// look if we did push something...
		if ( columnMap.size() == currTab ) {
			// no, we didn't. note that...
			FieldMapping empty;
			columnMap.push_back(empty);
		}

		// done 
		currTab++;
	} 

	if ( wantFields != num_fields ) {
		// we did not find all fields?
		// :(
		Error("wantFields != num_fields");
		return false;
	}
	

	this->num_fields = num_fields;
	
	// well, that seems to have worked...
	return true;
}

// read the entire file and send appropriate thingies back to InputMgr
bool InputReaderAscii::DoUpdate() {
	// TODO: all the stuff we need for a second reading.
	// *cough*
	//
	

	string line;
	while ( getline(*file, line ) ) {
		// split on tabs
		
		istringstream splitstream(line);
		string s;
	
		LogVal fields[num_fields];

		unsigned int currTab = 0;
		unsigned int currField = 0;
		while ( splitstream ) {
			if ( !getline(splitstream, s, '\t') )
				break;

			
			if ( currTab >= columnMap.size() ) {
				Error("Tabs in heading do not match tabs in data?");
				//disabled = true;
				return false;
			}

			FieldMapping currMapping = columnMap[currTab];
			currTab++;

			if ( currMapping.IsEmpty() ) {
				// well, that was easy
				continue;
			}

			if ( currField >= num_fields ) {
				Error("internal error - fieldnum greater as possible");
				return false;
			}

			LogVal val(currMapping.type, true);

			switch ( currMapping.type ) {
			case TYPE_STRING:
				val.val.string_val = new string(s);

			default:
				Error(Fmt("unsupported field format %d for %s", currMapping.type,
			 	 currMapping.name.c_str()));
				return false;
			}	

			currField++;
		}

		if ( currField != num_fields ) {
			Error("curr_field != num_fields in DoUpdate");
			return false;
		}

		// ok, now we have built our line. send it back to... whomever.

	}

	return true;
}
