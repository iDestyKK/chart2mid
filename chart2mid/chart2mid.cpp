/*
		CHART2MID C++
		Because we want to convert our charts back to midi after our hard work.

		The purpose of this console application is to convert chart files to a MIDI format almost exactly (if not, the same) the same as Rock Band 3's Format.
		This converter converts custom instruments (Edit the source code const variables) and many other things.

		NOTE THIS ONLY CONVERTS TIME SIGNATURES AND BPM CHANGES ATM. EXTREMELY INCOMPLETE!!!

		Update (2014年5月7日):
		-Reads [Song] for Deltatime.
		-Added TS and BPM scanning.
		-MIDI File Writing implemented.
		-Tested importing MIDI with FL Studio. Works flawlessly.

		By: Clara Eleanor Taylor

		Oh also this is the first C++ Application I wrote that uses header files! Notice that "midi_classes.h" there. :P
*/

#include <iostream>
#include <cmath>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

#include "midi_classes.h"

using namespace std;

//These are GLOBAL application settings. Change them if you want to add in extra difficulties, instruments, etc.
const int num_of_ins = 4;
const int num_of_difficulties = 4;

//Define all of the names here.
const string difficulties[num_of_difficulties] = { "Expert", "Hard", "Medium", "Easy" };
const string instruments[num_of_ins] = { "Single", "DoubleBass", "Drums", "Keyboard" };
const string corris_inst[num_of_ins] = { "PART GUITAR", "PART BASS", "PART DRUMS", "PART KEYS" };

//Tell the application what values are actually notes...
const unsigned char note_hex[num_of_difficulties][5] = { { 0x60, 0x61, 0x62, 0x63, 0x64 },
                                                         { 0x54, 0x55, 0x56, 0x57, 0x58 },
                                                         { 0x48, 0x49, 0x4A, 0x4B, 0x4C },
                                                         { 0x3C, 0x3D, 0x3E, 0x3F, 0x40 } };

string tostring(int integer) {
	string res;
	ostringstream convert;
	convert << integer;
	return convert.str();
}

byte bitstr_to_int(string bitstr) {
	byte integer = 0;
	for (int i = 0; i < bitstr.length(); i++)
		integer += pow((double)2, i) * (bitstr[i] == '1');
	return integer;
}

string to_hex(int number) {
	stringstream stream;
	stream << hex << number;
	return stream.str();
}

void Int_to_VLQ(int number, vector<byte> &vec) {
	//Count up how many bits this number will use.
	int _i;

	//Somewhat inefficient For Loop ftw
	for (_i = 0; number >= (2 << _i); _i++) {}

	//Add by 1.
	_i++;

	//Let's display those bits...
	string bitstr = "";
	for (int i = 0; i < _i; i++)
		bitstr += tostring((number & (1 << i)) != 0);

	//Initiate the byte count. This is necessary for many reasons.
	int bytecount = (_i / 7) + 1;
	byte bytecutoff = _i % 7;

	string* bytestr_of_VLQ = new string[bytecount];
	for (int i = 0; i < bytecount; i++) {
		for (int a = 7 * i; a <= 6 + (7 * i); a++) {
			if (a >= bitstr.length())
				bytestr_of_VLQ[i] += '0';
			else
				bytestr_of_VLQ[i] += bitstr[a];
		}
		bytestr_of_VLQ[i] += tostring(i != 0);
	}

	byte* bytes_of_VLQ = new byte[bytecount]; //Let's make some bytes... I think.
	for (int i = 0; i < bytecount; i++) {
		//Convert to integers... Well typedef bytes.
		bytes_of_VLQ[i] = bitstr_to_int(bytestr_of_VLQ[(bytecount - 1) - i]);
	}

	//Check for an unnecessary number... 0x80 can be the first number but it would mean absolutely nothing at all... Period.
	//On the plus side, it saves us an additional byte.
	if (bytes_of_VLQ[0] == 0x80) {
		for (int i = 0; i < bytecount - 1; i++) {
			//Shift them all down by 1.
			bytes_of_VLQ[i] = bytes_of_VLQ[i + 1];
		}
		bytecount -= 1;
	}

	for (int i = 0; i < bytecount; i++) {
		//cout << "0x" << to_hex((int)bytes_of_VLQ[i]) << " ";
		vec.push_back(bytes_of_VLQ[i]);
	}
}

void writeMTrkHeader(vector<byte> &vec) {
	byte MTrk[] = { 0x4D, 0x54, 0x72, 0x6B };
	vec.insert(vec.end(), begin(MTrk), end(MTrk));
}

void addTitleEventToVector(vector<byte> &vec, string text) {
	vec.push_back(0x03); //0x03 is Track Title Event.
	vec.push_back((byte)text.length()); //String length
	for (int i = 0; i < text.length(); i++)
		vec.push_back((byte)text[i]);
}

void addTextEventToVector(vector<byte> &vec, string text) {
	vec.push_back(0x01); //0x01 is Text Event.
	vec.push_back((byte)text.length()); //String length
	for (int i = 0; i < text.length(); i++)
		vec.push_back((byte)text[i]);
}

int main() {

	//Show the console our pretty little art.
	cout << "/---------------------------------------------------------\\" << endl
		<< "|*                                                       *|" << endl
		<< "|                  < Clara's Chart2Mid >                  |" << endl
		<< "|                     Version 1.0.0.0                     |" << endl
		<< "|                                                         |" << endl
		<< "|    For people who know what the hell they are doing.    |" << endl
		<< "|*                                                       *|" << endl
		<< "\\---------------------------------------------------------/\n\n";

	string path = "c:\\test\\notes.mid.chart";
	ifstream chart;
	chart.open(path);

	string line;
	//Read [Song] Information.
	getline(chart,line); //It is merely "[Song]"
	getline(chart,line); //It is merely a "{"...
	
	unsigned int deltatime, number_of_tracks, track_type, track_number;

	track_type = 0x01;
	track_number = 0;

	cout << "Reading [Song]..." << endl;

	//Let us begin by scanning files until it hits a "}".
	while (getline(chart,line) && line[0] != '}') {
		istringstream values;
		values.clear();
		values.str(line);
		//cout << line << endl;
		string firstfield, equals;
		unsigned int valueof;
		values >> firstfield >> equals >> valueof;
		//Now is the time where we check what variables are what. Afterwards, we will use this information for the MIDI.
		if (firstfield == "Resolution") {
			//Resolution = Delta-Time of MIDI. This is the only information we need for now.
			deltatime = valueof;
			cout << "Delta time: " << deltatime << endl;
		}
	}
	cout << "Track Type: ";
	switch (track_type) {
		case 0: cout << "Single Track MIDI" << endl; break;
		case 1: cout << "Multi Synchronous Track MIDI" << endl; break;
		case 2: cout << "Multi Asynchronous Track MIDI" << endl; break;
		default: cout << "Unknown Track Type MIDI" << endl; break;
	}

	//The next line should be "[SyncTrack]"...
	cout << "Reading [SyncTrack]..." << endl;
	getline(chart,line); //It is merely "[SyncTrack]"
	getline(chart,line); //It is merely a "{"...
	//Let us begin by scanning files until it hits a "}".

	unsigned int pos, value;
	string type, svalue;
	vector<SyncTrack> SyncEvents_in_chart;

	while (getline(chart,line) && line[0] != '}') {
		istringstream values;
		values.clear();
		values.str(line);
		string firstfield, equals;
		values >> pos >> equals >> type >> value;

		//Now is the time where we check what variables are what. Afterwards, we will use this information for the MIDI.
		if (type == "B") {
			SyncEvents_in_chart.push_back(SyncTrack(BPM(pos, value)));
		}
		if (type == "TS") {
			SyncEvents_in_chart.push_back(SyncTrack(TS(pos, value)));
		}
	}

	track_number++;

	//Afterwards is "EVENTS"
	getline(chart,line);
	bool events_exist = false;
	vector<Event> Events_in_chart;
	if (line == "[Events]" || line == "[EVENTS]") {
		cout << "Reading [Events]..." << endl;
		events_exist = true;

		while (getline(chart,line) && line[0] != '}') {
			istringstream values;
			values.clear();
			values.str(line);
			string firstfield, equals;
			values >> pos >> equals >> type >> svalue; //Now you know why I initialized "svalue" earlier. ;)
			
			//Now is the time where we check what variables are what. Afterwards, we will use this information for the MIDI.
			if (type == "E") {
				Events_in_chart.push_back(Event(pos, svalue));
			}
		}
		track_number++;
	}
	


	//Draw events... if necessary.
	/*for (int i = 0; i < SyncEvents_in_chart.size(); i++) {
		switch (SyncEvents_in_chart[i].getType())
		{
			case 0:
				cout << "BPM Change: "     << SyncEvents_in_chart[i].getBPM().getValue() << endl;
				break;
			case 1:
				cout << "Time Signature: " << SyncEvents_in_chart[i].getTS ().getValue() << endl;
				break;
		}
	}*/

	bool instrument_exists[num_of_ins];
	for (int i = 0; i < num_of_ins; i++) {
		instrument_exists[i] = false; //Just saying.
	}

	cout << endl << "Checking Instrument Availability..." << endl;
	ifstream dscan;
	dscan.open(path);
	while (getline(dscan,line)) {
		//Scan for difficulties
		if (line[0] == '[') {
			//It is a category... Let's see if it is events... or an actual note track.
			if (line != "[Song]" && line != "[SyncTrack]" && line != "[Events]") {
				//Bingo.
				string txt = line.substr(1,line.length() - 2);
				for (int i = 0; i < num_of_difficulties; i++) {
					if (txt.substr(0,difficulties[i].length()) == difficulties[i])
						txt = txt.substr(difficulties[i].length(),txt.length() - difficulties[i].length());
				}
				for (int i = 0; i < num_of_ins; i++) {
					if (txt == instruments[i])
						instrument_exists[i] = true;
				}
			}
		}
	}
	dscan.close();

	cout << endl << "Instruments Available:" << endl;
	for (int i = 0; i < num_of_ins; i++) {
		if (instrument_exists[i] == true) {
			cout << "  -" << instruments[i] << endl;
			track_number++;
		}
	}
	
	chart.close();
	cout << endl;
	cout << "- Begin Generating MIDI file -" << endl;

	//Write Header
	cout << "Generating Header Bytes..." << endl;
	vector<byte> fileContents;
	ofstream midi;
	midi.open("song.mid", ios::binary);
	byte header[] = { 0x4D, 0x54, 0x68, 0x64, 0x00, 0x00, 0x00, 0x06, track_type / 256, track_type % 256, track_number / 256, track_number % 256 , deltatime / 256, deltatime % 256 };
	fileContents.insert(fileContents.end(), begin(header), end(header));

	//This is where it gets a bit... complicated. Now we need an individual vector specifically for the SyncTrack Events. To do this, abuse classes and generate a MIDI Channel.
	cout << "Generating MIDI_EXPORT..." << endl;
	writeMTrkHeader(fileContents);
	vector<byte> MIDI_EXPORT;
	byte starter[] = { 0x00, 0xFF };
	MIDI_EXPORT.insert(MIDI_EXPORT.end(), begin(starter), end(starter));
	addTitleEventToVector(MIDI_EXPORT, "midi_export");
	//MIDI_EXPORT.insert(MIDI_EXPORT.end(), begin(channel_name), end(channel_name));
	unsigned int channel_size = 0;
	unsigned int prev_pos = 0;

	for (int i = 0; i < SyncEvents_in_chart.size(); i++) {
		if (i == 0) {
			MIDI_EXPORT.push_back(0x00);
		}
		else {
			Int_to_VLQ(SyncEvents_in_chart[i].getPos() - prev_pos, MIDI_EXPORT);
		}

		//Abusing SyncTrack class...
		unsigned int tempo;
		switch (SyncEvents_in_chart[i].getType())
		{
			case 0:
				//BPM change.
				MIDI_EXPORT.push_back(0xFF);
				MIDI_EXPORT.push_back(0x51);
				MIDI_EXPORT.push_back(0x03);
				tempo = (60000000000 / (unsigned int)SyncEvents_in_chart[i].getBPM().getValue());
				MIDI_EXPORT.push_back(tempo / (2 << 15));
				MIDI_EXPORT.push_back(tempo / (2 <<  7));
				MIDI_EXPORT.push_back(tempo % (2 <<  7));
				prev_pos = SyncEvents_in_chart[i].getPos();
				break;
			case 1:
				//Time Signature Change
				MIDI_EXPORT.push_back(0xFF);
				MIDI_EXPORT.push_back(0x58);
				MIDI_EXPORT.push_back(0x04);
				MIDI_EXPORT.push_back(SyncEvents_in_chart[i].getTS().getValue());
				MIDI_EXPORT.push_back(0x02);
				MIDI_EXPORT.push_back(0x18);
				MIDI_EXPORT.push_back(0x08);
				prev_pos = SyncEvents_in_chart[i].getPos();
				break;
		}
	}
	MIDI_EXPORT.push_back(0x00);
	MIDI_EXPORT.push_back(0xFF);
	MIDI_EXPORT.push_back(0x2F); //0x2F Indicates the end of the track.
	MIDI_EXPORT.push_back(0x00);

	//Now that we are done with writing the events... Kill the track and then add it to the main vector.
	byte csize[] = { MIDI_EXPORT.size() / (2 << 23)
		           , MIDI_EXPORT.size() / (2 << 15)
				   , MIDI_EXPORT.size() / (2 <<  7)
				   , MIDI_EXPORT.size() % (2 <<  7) };

	fileContents.insert(fileContents.end(), begin(csize), end(csize));
	fileContents.insert(fileContents.end(), MIDI_EXPORT.begin(), MIDI_EXPORT.end());

	if (events_exist) {
		//How about Track events?
		cout << "Generating EVENTS..." << endl;
		writeMTrkHeader(fileContents);
		vector<byte> EVENTS;
		unsigned int prev_pos = 0;
		byte starter[] = { 0x00, 0xFF };
		EVENTS.insert(EVENTS.end(), begin(starter), end(starter));
		addTitleEventToVector(EVENTS, "EVENTS");
		for (int i = 0; i < Events_in_chart.size(); i++) {
			Int_to_VLQ(Events_in_chart[i].getPos() - prev_pos, EVENTS);
			prev_pos = Events_in_chart[i].getPos();
			EVENTS.push_back(0xFF);
			string tmpstr = Events_in_chart[i].getText();
			tmpstr = '[' + tmpstr.substr(1,tmpstr.length() - 2) + ']';
			addTextEventToVector(EVENTS, tmpstr);
		}
		EVENTS.push_back(0x00);
		EVENTS.push_back(0xFF);
		EVENTS.push_back(0x2F); //0x2F Indicates the end of the track.
		EVENTS.push_back(0x00);

		byte csize[] = { EVENTS.size() / (2 << 23)
		               , EVENTS.size() / (2 << 15)
		               , EVENTS.size() / (2 <<  7)
		               , EVENTS.size() % (2 <<  7) };

		//Write events to MIDI File.
		fileContents.insert(fileContents.end(), begin(csize), end(csize));
		fileContents.insert(fileContents.end(), EVENTS.begin(), EVENTS.end());
	}


	cout << "Writing Data";
	for (int i = 0; i < fileContents.size(); i++) {
		midi << fileContents[i];
	}
	cout << " ...Successful!" << endl;
	midi.close();
	getchar();
}